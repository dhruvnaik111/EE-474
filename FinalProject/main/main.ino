#include <ESP32Servo.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

// --- Pins ---
const int servoPin = 13;
const int IN1 = 19;
const int IN2 = 18;
const int IN3 = 5;
const int IN4 = 17;
const int trigPin = 4;
const int echoPin = 2;

// --- Stepper Config ---
#define STEPS_PER_DEG       (2048.0 / 360.0)
#define STEP_DELAY_MS       4
#define STEPPER_MAX_DEG     90.0

// --- Servo Config ---
// 0  = horizontal
// 70 = 70 degrees up (maximum)
// No downward tilt available on this servo orientation
#define SERVO_PHYSICAL_OFFSET   0   // 0 is horizontal on this servo
#define SERVO_START_DEG         0   // Start at horizontal
#define SERVO_MIN_DEG           0   // Horizontal (no downward tilt)
#define SERVO_MAX_DEG          70   // 70 degrees up
#define SERVO_STEP_DEG         10   // Vertical step between horizontal sweeps
#define SERVO_SETTLE_MS       300   // Wait for servo to settle before reading

// --- Detection ---
#define TARGET_DIST_CM      30.0
#define LOST_DIST_CM        40.0

// --- System State ---
enum SystemState { SCANNING, LOCKED };
volatile SystemState systemState = SCANNING;

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

// --- Objects ---
Servo myServo;
const int stepsPerRev = 2048;
Stepper myStepper(stepsPerRev, IN1, IN3, IN2, IN4);
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

    if (systemState == LOCKED) {
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
// Sweeps from horizontal (0) up to 70 degrees.
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

  int stepperDir = 1;
  int currentTilt = SERVO_MIN_DEG; // Track which tilt row we are on

  while (true) {
    if (systemState == LOCKED) {
      releaseStepper();
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    Serial.println("[SCAN] Resuming dome sweep.");

    // Resume from currentTilt instead of always restarting at SERVO_MIN_DEG
    for (int tilt = currentTilt; tilt <= SERVO_MAX_DEG;
         tilt += SERVO_STEP_DEG) {

      if (systemState == LOCKED) {
        currentTilt = tilt; // Save exactly where we stopped
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
        currentTilt = tilt; // Save exactly where we stopped
        break;
      }

      stepperDir = -stepperDir;
      currentTilt = tilt + SERVO_STEP_DEG; // Advance saved position
    }

    if (systemState == LOCKED) {
      releaseStepper();
      continue;
    }

    // Full sweep complete — reset back to bottom for next pass
    Serial.println("[SCAN] Sweep complete. Returning home.");
    servoWrite(SERVO_START_DEG);
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    servoAngleDeg = SERVO_START_DEG;
    xSemaphoreGive(stateMutex);
    vTaskDelay(pdMS_TO_TICKS(SERVO_SETTLE_MS));

    stepperToCenter();
    stepperDir  = 1;
    currentTilt = SERVO_MIN_DEG; // Only reset after a full sweep completes
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// =====================
// --- Sensor Task ---
// Core 0. Polls ultrasonic every 80ms.
// =====================
void sensorTask(void *pvParameters) {
  while (true) {
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
      Serial.println("[SENSOR] Target lost — resuming scan.");
    }

    xSemaphoreGive(stateMutex);

    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

// =====================
// --- LCD Task ---
// Core 0. Updates every 200ms.
// SCANNING: H angle, V tilt, distance
// LOCKED:   TARGET alert + coords + distance
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
    if (state == SCANNING) {
      coordsCaptured = false;
    }

    if (state == SCANNING) {
      // Row 0: H stepper angle and V servo tilt
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
      // Row 0: alert
      lcd.setCursor(0, 0);
      lcd.print("** TARGET **    ");

      // Row 1: locked H/V coords + distance
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

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Wire.begin(8, 9);
  lcd.init();
  delay(2);
  lcd.backlight();
  lcd.clear();

  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(SERVO_START_DEG); // Direct write for init — 0 = horizontal

  myStepper.setSpeed(8);
  releaseStepper();

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
  xTaskCreatePinnedToCore(scanTask,   "ScanTask",   8192, NULL, 2,
                          &scanTaskHandle,   1);
  // Sensor + LCD on Core 0
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 2048, NULL, 3,
                          &sensorTaskHandle, 0);
  xTaskCreatePinnedToCore(lcdTask,    "LcdTask",    2048, NULL, 1,
                          &lcdTaskHandle,    0);

  Serial.println("Sentry online. Beginning dome sweep.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}