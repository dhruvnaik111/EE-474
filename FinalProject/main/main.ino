/**
 * @file main.ino
 * @authors Dhruv Naik, Ethan Le
 * @date 03/12/2026
 * @brief Entry point and FreeRTOS task implementations for the 2-axis
 *        ultrasonic sentry system.
 * @details Initialises all hardware peripherals, FreeRTOS primitives, hardware
 *          timers, and spawns all tasks across both ESP32-S3 cores. All hardware
 *          helper functions, ISRs, pin definitions, macros, and shared state
 *          are defined in helpers.h.
 *
 *          Hardware summary:
 *            - Stepper motor (28BYJ-48): horizontal sweep ±90 degrees
 *            - Servo motor (SG90):       vertical tilt 0-70 degrees (0 = horizontal)
 *            - HC-SR04 ultrasonic:       target detection within 30cm
 *            - LiquidCrystal I2C LCD:    16x2 status display
 *            - Active buzzer (GPIO 1):   PWM alert, auto-off after 2 seconds
 *            - E-stop button (GPIO 6):   interrupt-driven emergency halt
 *
 *          FreeRTOS task summary:
 *            Core 0 — servoWriteTask (P3), sensorTask (P3, 128Hz),
 *                     lcdTask (P1), estopTask (P4, 50Hz)
 *            Core 1 — scanTask (P2)
 *
 *          Timer summary:
 *            Hardware Timer           — buzzTimerISR fires every 500ms for buzzer toggle
 *            Hardware Timer 1         — explicitly allocated to ESP32PWM for servo PWM signal
 *            LEDC Peripheral Timer    — internally allocated by ledcAttach for the buzzer tone
 */

// ==================== Includes ====================
#include "helpers.h"

// ==================== Task Handles ====================
TaskHandle_t scanTaskHandle       = NULL;
TaskHandle_t sensorTaskHandle     = NULL;
TaskHandle_t lcdTaskHandle        = NULL;
TaskHandle_t servoWriteTaskHandle = NULL;
TaskHandle_t estopTaskHandle      = NULL;

// ==================== Function Prototypes ====================
void servoWriteTask(void *pvParameters);
void sensorTask(void *pvParameters);
void estopTask(void *pvParameters);
void scanTask(void *pvParameters);
void lcdTask(void *pvParameters);

// ==================== FreeRTOS Task Implementations ====================
/**
 * @brief Receives servo angle commands from servoQueue and writes to servo.
 * @details Core 0, Priority 3. Sole owner of myServo.write() to avoid
 *          ESP32Servo cross-core threading issues. Blocks indefinitely on
 *          queue receive, consuming no CPU while idle.
 * @param pvParameters Unused FreeRTOS task parameter.
 */
void servoWriteTask(void *pvParameters) {
  int angle;

  while (true) {
    if (xQueueReceive(servoQueue, &angle, portMAX_DELAY)) {
      myServo.write(angle);
    }
  }
}

/**
 * @brief Polls the ultrasonic sensor at 128Hz and manages SCANNING/LOCKED state.
 * @details Core 0, Priority 3. Uses vTaskDelayUntil for precise 128Hz timing.
 *          Posts distance readings to distQueue via xQueueOverwrite so lcdTask
 *          always receives the freshest value without blocking. Sets buzzEnabled
 *          when a target is acquired to trigger the hardware timer buzzer.
 * @param pvParameters Unused FreeRTOS task parameter.
 */
void sensorTask(void *pvParameters) {
  while (true) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (systemState != ESTOP) {
      float dist = getStableDistance();

      // Overwrite queue — LCD always gets freshest reading, no blocking
      xQueueOverwrite(distQueue, &dist);

      xSemaphoreTake(stateMutex, portMAX_DELAY);

      if (systemState == SCANNING && dist > 0 && dist <= TARGET_DIST_CM) {
        systemState   = LOCKED;
        buzzEnabled   = true;   // Trigger timer-driven 2-second buzzer
        buzzTickCount = 0;

        Serial.print("[SENSOR] TARGET ACQUIRED at ");
        Serial.print(dist, 1);
        Serial.println("cm — LOCKED.");

      } else if (systemState == LOCKED && (dist < 0 || dist > LOST_DIST_CM)) {
        systemState   = SCANNING;
        buzzEnabled   = false;  // Cancel buzzer if target clears before 2s
        buzzTickCount = 0;
        ledcWrite(buzzerPin, 0);
        Serial.println("[SENSOR] Target lost — resuming scan.");
      }

      xSemaphoreGive(stateMutex);

    } else {
      // E-stopped — push invalid reading so LCD shows --
      float invalid = -1.0;
      xQueueOverwrite(distQueue, &invalid);
    }

    // Precise 128Hz timing using absolute wake time
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_TASK_MS));
  }
}

/**
 * @brief Monitors the e-stop button at 50Hz and manages the ESTOP state.
 * @details Core 0, Priority 4 (highest). Uses vTaskDelayUntil for precise
 *          50Hz timing. On press: transitions to ESTOP, homes servo to
 *          horizontal, releases stepper coils, and silences the buzzer.
 *          The stepper horizontal angle is intentionally preserved so the
 *          sweep can resume from the correct physical position on release.
 *          On release: transitions back to SCANNING.
 * @param pvParameters Unused FreeRTOS task parameter.
 */
void estopTask(void *pvParameters) {
  bool wasActive = false;

  while (true) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (estopActive && !wasActive) {
      wasActive = true;
      Serial.println("[ESTOP] Emergency stop triggered!");

      xSemaphoreTake(stateMutex, portMAX_DELAY);
      systemState   = ESTOP;
      // stepperAngle intentionally NOT reset — horizontal position preserved
      servoAngleDeg = SERVO_START_DEG;
      xSemaphoreGive(stateMutex);

      // Silence buzzer immediately
      buzzEnabled   = false;
      buzzTickCount = 0;
      ledcWrite(buzzerPin, 0);

      // Release stepper coils without altering angle tracking
      releaseStepper();

      // Home servo to horizontal via queue
      int homeAngle = SERVO_START_DEG + SERVO_PHYSICAL_OFFSET;
      xQueueSend(servoQueue, &homeAngle, portMAX_DELAY);

      Serial.println("[ESTOP] Halted. Waiting for button release...");

    } else if (!estopActive && wasActive) {
      wasActive = false;
      Serial.println("[ESTOP] Released. Resuming from current horizontal position.");

      xSemaphoreTake(stateMutex, portMAX_DELAY);
      systemState   = SCANNING;
      // stepperAngle unchanged — motor resumes from physical position
      servoAngleDeg = SERVO_START_DEG;
      xSemaphoreGive(stateMutex);

      // Confirm servo at home — send again to be safe
      int homeAngle = SERVO_START_DEG + SERVO_PHYSICAL_OFFSET;
      xQueueSend(servoQueue, &homeAngle, portMAX_DELAY);
      ledcWrite(buzzerPin, 0);
    }

    // Precise 50Hz timing using absolute wake time
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ESTOP_TASK_MS));
  }
}

/**
 * @brief Performs the boustrophedon 3D dome sweep using stepper and servo.
 * @details Core 1, Priority 2. Sweeps the servo vertically row by row while
 *          the stepper scans the full horizontal arc at each row, alternating
 *          direction each row (snake/boustrophedon pattern) to avoid returning
 *          to center between rows. Pauses immediately on LOCKED or ESTOP.
 *          Resumes from the last saved vertical row after LOCKED clears.
 *          Resets vertical sweep to the start after ESTOP clears, while
 *          preserving the horizontal stepper position throughout.
 * @param pvParameters Unused FreeRTOS task parameter.
 */
void scanTask(void *pvParameters) {
  myStepper.setSpeed(8);

  // Startup servo test — confirms physical movement before sweep begins
  Serial.println("[SCAN] Servo test...");
  servoWrite(SERVO_START_DEG);
  vTaskDelay(pdMS_TO_TICKS(500));
  servoWrite(SERVO_MAX_DEG);
  vTaskDelay(pdMS_TO_TICKS(600));
  servoWrite(SERVO_START_DEG);
  vTaskDelay(pdMS_TO_TICKS(600));
  Serial.println("[SCAN] Servo test complete.");

  int stepperDir  = 1;             // +1 = sweep right first, -1 = sweep left
  int currentTilt = SERVO_MIN_DEG; // Tracks current vertical row for resume

  while (true) {
    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();

      // On e-stop reset vertical sweep state only — horizontal preserved
      if (systemState == ESTOP) {
        currentTilt = SERVO_MIN_DEG;
        stepperDir  = 1;
      }

      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    Serial.println("[SCAN] Resuming dome sweep.");

    // Resume from currentTilt — preserves vertical position across lock/unlock
    for (int tilt = currentTilt; tilt <= SERVO_MAX_DEG; tilt += SERVO_STEP_DEG) {

      if (systemState == LOCKED) {
        currentTilt = tilt; // Save row so we resume here after unlock
        break;
      }
      if (systemState == ESTOP) {
        currentTilt = SERVO_MIN_DEG; // Full vertical reset on e-stop
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

      // Sweep full horizontal arc in current direction (snake pattern)
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

      stepperDir  = -stepperDir;           // Flip horizontal direction each row
      currentTilt = tilt + SERVO_STEP_DEG; // Advance saved vertical position
    }

    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();
      continue;
    }

    // Full dome sweep complete — return both axes to home position
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

/**
 * @brief Updates the LCD display at 200ms intervals.
 * @details Core 0, Priority 1 (lowest). Reads the latest distance from
 *          distQueue using a non-blocking peek and reads system state from
 *          shared variables protected by stateMutex. Displays three different
 *          layouts depending on state: SCANNING shows live H/V angles and
 *          distance, LOCKED shows a target alert with the coordinates where
 *          the target was first detected, ESTOP shows a halt message.
 * @param pvParameters Unused FreeRTOS task parameter.
 */
void lcdTask(void *pvParameters) {
  float lockedStepAngle  = 0.0;
  int   lockedServoAngle = SERVO_START_DEG;
  float lockedDist       = 0.0;
  bool  coordsCaptured   = false;
  float dist             = -1.0;

  while (true) {
    // Non-blocking peek — keeps last value if no new reading available
    xQueuePeek(distQueue, &dist, 0);

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    float       sAngle = stepperAngle;
    int         tilt   = servoAngleDeg;
    SystemState state  = systemState;
    xSemaphoreGive(stateMutex);

    // Capture lock coordinates once on SCANNING -> LOCKED transition
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
      lcd.setCursor(0, 0);
      lcd.print("!! ESTOP !!     ");
      lcd.setCursor(0, 1);
      lcd.print("Release to resume");

    } else if (state == SCANNING) {
      // Row 0: live horizontal stepper angle and vertical servo tilt
      lcd.setCursor(0, 0);
      lcd.print("H:");
      if (sAngle >= 0) lcd.print(" ");
      lcd.print((int)sAngle);
      lcd.print((char)223); // Degree symbol
      lcd.print(" V:+");
      lcd.print(tilt);
      lcd.print((char)223);
      lcd.print("  ");

      // Row 1: live ultrasonic distance reading
      lcd.setCursor(0, 1);
      lcd.print("Dist: ");
      if (dist > 0) {
        lcd.print(dist, 0);
        lcd.print("cm      ");
      } else {
        lcd.print("--cm    ");
      }

    } else {
      // LOCKED — show alert and coordinates where target was first detected
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

// ==================== Setup ====================
/**
 * @brief Initialise all hardware, FreeRTOS primitives, timers, and tasks.
 * @details Runs once on boot. Configures GPIO pins, LCD, servo, stepper,
 *          buzzer PWM, hardware timers, ISRs, queues, mutex, and spawns
 *          all FreeRTOS tasks pinned to their respective cores.
 */
void setup() {
  Serial.begin(115200);

  // Configure ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Configure e-stop with internal pull-up — interrupt fires on press and release
  pinMode(estopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(estopPin), estopISR, CHANGE);

  // Initialise LCD on I2C pins 8 (SDA) and 9 (SCL)
  Wire.begin(8, 9);
  lcd.init();
  delay(2);
  lcd.backlight();
  lcd.clear();

  // Initialise servo on Timer 1 (Timer 0 reserved for buzzer ISR)
  ESP32PWM::allocateTimer(1);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);

  myServo.write(SERVO_START_DEG); // Physical 0 = horizontal on this servo

  // Initialise stepper motor
  myStepper.setSpeed(8);
  releaseStepper();

  // Initialise buzzer via PWM (ledcAttach uses Timer 2 internally)
  ledcAttach(buzzerPin, BUZZ_FREQ, BUZZ_RESOLUTION);
  ledcWrite(buzzerPin, 0); // Start silent

  // Hardware Timer 0 — buzzer toggle ISR fires every 500ms
  // Auto-silences after BUZZ_DURATION_TICKS (2 seconds) via ISR tick counter
  buzzTimer = timerBegin(1000000);         // 1 MHz tick resolution
  timerAttachInterrupt(buzzTimer, &buzzTimerISR);
  timerAlarm(buzzTimer, 500000, true, 0);  // 500ms period, auto-reload enabled

  // Create inter-task communication queues
  servoQueue = xQueueCreate(8, sizeof(int));   // Servo angle commands
  distQueue  = xQueueCreate(1, sizeof(float)); // Latest distance (size 1 for overwrite)

  // Create shared state mutex
  stateMutex = xSemaphoreCreateMutex();

  lcd.setCursor(0, 0);
  lcd.print("Sentry Boot...  ");
  delay(1000);
  lcd.clear();

  // ---- Spawn FreeRTOS tasks ----

  // Core 0 — sensor polling, display, servo writer, e-stop monitor
  xTaskCreatePinnedToCore(servoWriteTask, "ServoWrite", 2048, NULL, 3,
                          &servoWriteTaskHandle, 0);
  xTaskCreatePinnedToCore(sensorTask,     "SensorTask", 4096, NULL, 3,
                          &sensorTaskHandle,     0); // 128Hz
  xTaskCreatePinnedToCore(lcdTask,        "LcdTask",    2048, NULL, 1,
                          &lcdTaskHandle,        0); // Lowest priority
  xTaskCreatePinnedToCore(estopTask,      "EStopTask",  2048, NULL, 4,
                          &estopTaskHandle,      0); // 50Hz, highest priority

  // Core 1 — motor sweep isolated to prevent conflicts with Core 0 tasks
  xTaskCreatePinnedToCore(scanTask,       "ScanTask",   8192, NULL, 2,
                          &scanTaskHandle,       1);

  Serial.println("Sentry online. Beginning dome sweep.");
}

// ==================== Main Loop ====================

/**
 * @brief Arduino main loop — intentionally idle.
 * @details All system behaviour is managed by FreeRTOS tasks spawned in
 *          setup(). The Arduino loop task runs at the lowest FreeRTOS
 *          priority and yields every second to avoid consuming CPU time.
 */
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
