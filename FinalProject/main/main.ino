#include <ESP32Servo.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

// --- Pins ---
const int servoPin   = 13;
const int IN1        = 19;
const int IN2        = 18;
const int IN3        = 5;
const int IN4        = 17;
const int trigPin    = 4;
const int echoPin    = 2;
const int buzzerPin  = 1;
const int estopPin   = 6;

// --- Stepper Config ---
#define STEPS_PER_DEG       (2048.0 / 360.0)
#define STEP_DELAY_MS       4
#define STEPPER_MAX_DEG     90.0

// --- Servo Config ---
#define SERVO_PHYSICAL_OFFSET   0
#define SERVO_START_DEG         0
#define SERVO_MIN_DEG           0
#define SERVO_MAX_DEG          70
#define SERVO_STEP_DEG         10
#define SERVO_SETTLE_MS       300

// --- Buzzer Config ---
#define BUZZ_FREQ        2000
#define BUZZ_RESOLUTION  8
#define BUZZ_DUTY_ON     128
#define BUZZ_INTERVAL_MS 500

// --- Detection ---
#define TARGET_DIST_CM      30.0
#define LOST_DIST_CM        40.0

// --- E-Stop Debounce ---
#define DEBOUNCE_MS         50    // Minimum ms between valid button events

// --- System State ---
enum SystemState { SCANNING, LOCKED, ESTOP };
volatile SystemState systemState     = SCANNING;
volatile SystemState preStopState    = SCANNING; // State before e-stop
volatile bool        estopActive     = false;
volatile unsigned long lastDebounceTime = 0;

// --- Shared State ---
volatile float stepperAngle  = 0.0;
volatile int   servoAngleDeg = SERVO_START_DEG;
volatile float lastDist      = -1.0;
SemaphoreHandle_t stateMutex;

// --- Servo Queue ---
QueueHandle_t servoQueue;

// --- Task Handles ---
TaskHandle_t scanTaskHandle       = NULL;
TaskHandle_t sensorTaskHandle     = NULL;
TaskHandle_t lcdTaskHandle        = NULL;
TaskHandle_t servoWriteTaskHandle = NULL;
TaskHandle_t estopTaskHandle      = NULL;

// --- Objects ---
Servo myServo;
const int stepsPerRev = 2048;
Stepper myStepper(stepsPerRev, IN1, IN3, IN2, IN4);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================
// --- E-Stop ISR ---
// Triggered on both RISING and FALLING edge so we catch
// press and release. Debounced by ignoring events within
// DEBOUNCE_MS of the last valid event.
// IRAM_ATTR keeps the ISR in fast IRAM on ESP32.
// =====================
void IRAM_ATTR estopISR() {
  unsigned long now = millis();
  if (now - lastDebounceTime < DEBOUNCE_MS) return; // Ignore bounce
  lastDebounceTime = now;

  // Button pressed (LOW because INPUT_PULLUP) — engage e-stop
  if (digitalRead(estopPin) == LOW) {
    estopActive = true;
  } else {
    // Button released — clear e-stop
    estopActive = false;
  }
}

// =====================
// --- E-Stop Task ---
// Core 0. Watches estopActive flag set by ISR.
// On press: saves current state, resets everything to start.
// On release: resumes scanning from the beginning.
// =====================
void estopTask(void *pvParameters) {
  bool wasActive = false;

  while (true) {
    if (estopActive && !wasActive) {
      wasActive = true;

      Serial.println("[ESTOP] Emergency stop triggered!");

      // Save what we were doing and force ESTOP state
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      preStopState = systemState;
      systemState  = ESTOP;
      xSemaphoreGive(stateMutex);

      // Silence buzzer immediately
      ledcWrite(buzzerPin, 0);

      // Release stepper coils
      releaseStepper();

      // Return servo to horizontal
      int homeAngle = SERVO_START_DEG + SERVO_PHYSICAL_OFFSET;
      xQueueSend(servoQueue, &homeAngle, portMAX_DELAY);

      // Reset all positional tracking to start state
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      stepperAngle  = 0.0;
      servoAngleDeg = SERVO_START_DEG;
      lastDist      = -1.0;
      xSemaphoreGive(stateMutex);

      // Drive stepper back to center
      // We do a blind step-back using the last known angle
      // since stepperToCenter() relies on shared state we just reset
      // Note: stepperAngle was reset to 0 above so we can't use it —
      // instead we just release and let the scan task restart from center
      // on resume. For a hard return, keep a separate raw step counter.
      releaseStepper();

      Serial.println("[ESTOP] System halted. Waiting for button release...");

    } else if (!estopActive && wasActive) {
      wasActive = false;

      Serial.println("[ESTOP] Released. Restarting from scan.");

      // Reset to SCANNING from the beginning
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      systemState   = SCANNING;
      stepperAngle  = 0.0;
      servoAngleDeg = SERVO_START_DEG;
      xSemaphoreGive(stateMutex);

      // Return servo to start
      int homeAngle = SERVO_START_DEG + SERVO_PHYSICAL_OFFSET;
      xQueueSend(servoQueue, &homeAngle, portMAX_DELAY);

      ledcWrite(buzzerPin, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // Poll at 20ms — well above debounce threshold
  }
}

// =====================
// --- Servo Helper ---
// =====================
void servoWrite(int logicalDeg) {
  int physical = constrain(logicalDeg + SERVO_PHYSICAL_OFFSET, 0, 180);
  xQueueSend(servoQueue, &physical, portMAX_DELAY);
}

// =====================
// --- Servo Write Task ---
// Core 0. Sole owner of myServo.write().
// =====================
void servoWriteTask(void *pvParameters) {
  int angle;
  while (true) {
    if (xQueueReceive(servoQueue, &angle, portMAX_DELAY)) {
      myServo.write(angle);
    }
  }
}

// =====================
// --- Stepper Helpers ---
// =====================
void releaseStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

bool stepDegrees(float degrees) {
  int steps = (int)(degrees * STEPS_PER_DEG);
  int dir   = (steps > 0) ? 1 : -1;
  steps     = abs(steps);

  for (int s = 0; s < steps; s++) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    float angle = stepperAngle;
    xSemaphoreGive(stateMutex);

    float newAngle = angle + dir * (360.0 / 2048.0);
    if (newAngle > STEPPER_MAX_DEG || newAngle < -STEPPER_MAX_DEG) {
      releaseStepper();
      return false;
    }

    myStepper.step(dir);

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    stepperAngle = newAngle;
    xSemaphoreGive(stateMutex);

    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));

    // Abort mid-step if locked or e-stopped
    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();
      return false;
    }
  }

  releaseStepper();
  return true;
}

void stepperToCenter() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  float angle = stepperAngle;
  xSemaphoreGive(stateMutex);

  Serial.print("[MOTOR] Returning to center from ");
  Serial.print(angle, 1);
  Serial.println(" deg");

  stepDegrees(-angle);

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  stepperAngle = 0.0;
  xSemaphoreGive(stateMutex);

  releaseStepper();
}

// =====================
// --- Ultrasonic Helper ---
// =====================
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1.0;
  return (duration / 2.0) * 0.0343;
}

float getStableDistance() {
  float total = 0;
  int   valid = 0;
  for (int i = 0; i < 3; i++) {
    float d = getDistance();
    if (d > 0) { total += d; valid++; }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return (valid > 0) ? total / valid : -1.0;
}

// =====================
// --- Scan Task ---
// Core 1. Boustrophedon dome sweep.
// Pauses on LOCKED or ESTOP, resumes from last tilt on LOCKED,
// restarts from beginning on ESTOP release.
// =====================
void scanTask(void *pvParameters) {
  myStepper.setSpeed(8);

  // Startup servo test
  Serial.println("[SCAN] Servo test...");
  servoWrite(SERVO_START_DEG);
  vTaskDelay(pdMS_TO_TICKS(500));
  servoWrite(SERVO_MAX_DEG);
  vTaskDelay(pdMS_TO_TICKS(600));
  servoWrite(SERVO_START_DEG);
  vTaskDelay(pdMS_TO_TICKS(600));
  Serial.println("[SCAN] Servo test complete.");

  int stepperDir  = 1;
  int currentTilt = SERVO_MIN_DEG;

  while (true) {
    // Pause on LOCKED or ESTOP
    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();

      // If e-stopped, reset sweep position for clean restart
      if (systemState == ESTOP) {
        currentTilt = SERVO_MIN_DEG;
        stepperDir  = 1;
      }

      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    Serial.println("[SCAN] Resuming dome sweep.");

    for (int tilt = currentTilt; tilt <= SERVO_MAX_DEG;
         tilt += SERVO_STEP_DEG) {

      // Pause on LOCKED or ESTOP mid-sweep
      if (systemState == LOCKED) {
        currentTilt = tilt;
        break;
      }
      if (systemState == ESTOP) {
        currentTilt = SERVO_MIN_DEG; // Reset on e-stop
        stepperDir  = 1;
        break;
      }

      servoWrite(tilt);
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      servoAngleDeg = tilt;
      xSemaphoreGive(stateMutex);
      vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));

      Serial.print("[SCAN] Tilt: +");
      Serial.print(tilt);
      Serial.println(" deg");

      float sweepDeg = stepperDir * STEPPER_MAX_DEG * 2;
      stepDegrees(sweepDeg);

      if (systemState == LOCKED) {
        currentTilt = tilt;
        break;
      }
      if (systemState == ESTOP) {
        currentTilt = SERVO_MIN_DEG;
        stepperDir  = 1;
        break;
      }

      stepperDir  = -stepperDir;
      currentTilt = tilt + SERVO_STEP_DEG;
    }

    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();
      continue;
    }

    // Full sweep complete — return home
    Serial.println("[SCAN] Sweep complete. Returning home.");
    servoWrite(SERVO_START_DEG);
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    servoAngleDeg = SERVO_START_DEG;
    xSemaphoreGive(stateMutex);
    vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));

    stepperToCenter();
    stepperDir  = 1;
    currentTilt = SERVO_MIN_DEG;
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// =====================
// --- Sensor Task ---
// Core 0. Polls ultrasonic every 80ms.
// Skips detection while e-stopped.
// =====================
void sensorTask(void *pvParameters) {
  bool buzzerState = false;
  unsigned long lastBuzzTime = 0;

  while (true) {
    // Don't poll sensor while e-stopped
    if (systemState == ESTOP) {
      ledcWrite(buzzerPin, 0);
      buzzerState = false;
      vTaskDelay(pdMS_TO_TICKS(80));
      continue;
    }

    float dist = getStableDistance();

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    lastDist = dist;

    if (systemState == SCANNING && dist > 0 && dist <= TARGET_DIST_CM) {
      systemState = LOCKED;
      Serial.print("[SENSOR] TARGET ACQUIRED at ");
      Serial.print(dist, 1);
      Serial.println("cm — LOCKED.");

    } else if (systemState == LOCKED && (dist < 0 || dist > LOST_DIST_CM)) {
      systemState = SCANNING;
      ledcWrite(buzzerPin, 0);
      buzzerState = false;
      Serial.println("[SENSOR] Target lost — resuming scan.");
    }

    xSemaphoreGive(stateMutex);

    // PWM buzz on/off while locked
    if (systemState == LOCKED) {
      unsigned long now = millis();
      if (now - lastBuzzTime >= BUZZ_INTERVAL_MS) {
        buzzerState = !buzzerState;
        ledcWrite(buzzerPin, buzzerState ? BUZZ_DUTY_ON : 0);
        lastBuzzTime = now;
      }
    } else {
      ledcWrite(buzzerPin, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

// =====================
// --- LCD Task ---
// Core 0. Updates every 200ms.
// SCANNING: H angle, V tilt, distance
// LOCKED:   TARGET alert + coords + distance
// ESTOP:    Emergency stop message
// =====================
void lcdTask(void *pvParameters) {
  float lockedStepAngle  = 0.0;
  int   lockedServoAngle = SERVO_START_DEG;
  float lockedDist       = 0.0;
  bool  coordsCaptured   = false;

  while (true) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    float       sAngle = stepperAngle;
    int         tilt   = servoAngleDeg;
    float       dist   = lastDist;
    SystemState state  = systemState;
    xSemaphoreGive(stateMutex);

    if (state == LOCKED && !coordsCaptured) {
      lockedStepAngle  = sAngle;
      lockedServoAngle = tilt;
      lockedDist       = dist;
      coordsCaptured   = true;
    }
    if (state == SCANNING || state == ESTOP) {
      coordsCaptured = false;
    }

    if (state == ESTOP) {
      // E-stop display
      lcd.setCursor(0, 0);
      lcd.print("!! ESTOP !!     ");
      lcd.setCursor(0, 1);
      lcd.print("Release to resume");

    } else if (state == SCANNING) {
      // Row 0: H and V angles
      lcd.setCursor(0, 0);
      lcd.print("H:");
      if (sAngle >= 0) lcd.print(" ");
      lcd.print((int)sAngle);
      lcd.print((char)223);
      lcd.print(" V:+");
      lcd.print(tilt);
      lcd.print((char)223);
      lcd.print("  ");

      // Row 1: distance
      lcd.setCursor(0, 1);
      lcd.print("Dist: ");
      if (dist > 0) {
        lcd.print(dist, 0);
        lcd.print("cm      ");
      } else {
        lcd.print("--cm    ");
      }

    } else {
      // LOCKED
      lcd.setCursor(0, 0);
      lcd.print("** TARGET **    ");

      lcd.setCursor(0, 1);
      lcd.print("H:");
      if (lockedStepAngle >= 0) lcd.print("+");
      lcd.print((int)lockedStepAngle);
      lcd.print(" V:+");
      lcd.print(lockedServoAngle);
      lcd.print(" ");
      lcd.print(lockedDist, 0);
      lcd.print("cm");
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// =====================
void setup() {
  Serial.begin(115200);

  pinMode(trigPin,  OUTPUT);
  pinMode(echoPin,  INPUT);

  // E-stop button — internal pullup, interrupt on both edges for press + release
  pinMode(estopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(estopPin), estopISR, CHANGE);

  // LCD
  Wire.begin(8, 9);
  lcd.init();
  delay(2);
  lcd.backlight();
  lcd.clear();

  // Servo
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(SERVO_START_DEG);

  // Stepper
  myStepper.setSpeed(8);
  releaseStepper();

  // Buzzer
  ledcAttach(buzzerPin, BUZZ_FREQ, BUZZ_RESOLUTION);
  ledcWrite(buzzerPin, 0);

  // FreeRTOS primitives
  servoQueue = xQueueCreate(8, sizeof(int));
  stateMutex = xSemaphoreCreateMutex();

  lcd.setCursor(0, 0);
  lcd.print("Sentry Boot...  ");
  delay(1000);
  lcd.clear();

  // ServoWrite on Core 0
  xTaskCreatePinnedToCore(servoWriteTask, "ServoWrite", 2048, NULL, 3,
                          &servoWriteTaskHandle, 0);
  // Scan on Core 1
  xTaskCreatePinnedToCore(scanTask,       "ScanTask",   8192, NULL, 2,
                          &scanTaskHandle,       1);
  // Sensor + LCD + EStop on Core 0
  xTaskCreatePinnedToCore(sensorTask,     "SensorTask", 2048, NULL, 3,
                          &sensorTaskHandle,     0);
  xTaskCreatePinnedToCore(lcdTask,        "LcdTask",    2048, NULL, 1,
                          &lcdTaskHandle,        0);
  xTaskCreatePinnedToCore(estopTask,      "EStopTask",  2048, NULL, 4,
                          &estopTaskHandle,      0);

  Serial.println("Sentry online. Beginning dome sweep.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}