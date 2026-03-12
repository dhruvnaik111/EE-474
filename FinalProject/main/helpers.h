/**
 * @file helpers.h
 * @authors Dhruv Naik, Ethan Le
 * @date 03/08/2026
 * @brief Hardware helper functions, ISRs, pin definitions, configuration macros,
 *        and shared state declarations for the 2-axis ultrasonic sentry system.
 * @details This header is included by main.ino. It defines all hardware pin
 *          assignments, tuning constants, the system state enum, all globally
 *          shared volatile variables, FreeRTOS primitive handles, hardware
 *          object declarations, and all non-FreeRTOS helper function
 *          implementations. FreeRTOS task implementations live in main.ino.
 */

#ifndef HELPERS_H
#define HELPERS_H

// ==================== Includes ====================
#include <Arduino.h>
#include <ESP32Servo.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

// ==================== Pin Definitions ====================
const int servoPin  = 13;   ///< Servo signal pin
const int IN1       = 19;   ///< Stepper coil 1
const int IN2       = 18;   ///< Stepper coil 2
const int IN3       = 5;    ///< Stepper coil 3
const int IN4       = 17;   ///< Stepper coil 4
const int trigPin   = 4;    ///< HC-SR04 trigger pin
const int echoPin   = 2;    ///< HC-SR04 echo pin
const int buzzerPin = 1;    ///< Active buzzer PWM pin
const int estopPin  = 6;    ///< Emergency stop button pin

// ==================== Stepper Configuration ====================
#define STEPS_PER_DEG       (2048.0 / 360.0)  ///< Steps per degree for 28BYJ-48
#define STEP_DELAY_MS       4                  ///< Delay between each step in ms
#define STEPPER_MAX_DEG     90.0               ///< Max horizontal sweep each side of center

// ==================== Servo Configuration ====================
/// Physical 0 degrees on this servo is horizontal — no offset required
#define SERVO_PHYSICAL_OFFSET   0
#define SERVO_START_DEG         0    ///< Logical horizontal start position
#define SERVO_MIN_DEG           0    ///< Lowest tilt (horizontal, no downward travel)
#define SERVO_MAX_DEG          70    ///< Highest tilt (70 degrees above horizontal)
#define SERVO_STEP_DEG         10    ///< Degrees between each vertical sweep row
#define SERVO_SETTLE_MS       300    ///< ms to wait for servo to physically reach position

// ==================== Buzzer Configuration ====================
#define BUZZ_FREQ            2000    ///< Buzzer tone frequency in Hz
#define BUZZ_RESOLUTION      8      ///< PWM resolution in bits (0-255 range)
#define BUZZ_DUTY_ON         128    ///< 50% duty cycle when buzzer is active
#define BUZZ_DURATION_TICKS  4      ///< ISR firings before auto-off (4 x 500ms = 2s)

// ==================== Detection Configuration ====================
#define TARGET_DIST_CM      30.0    ///< Distance threshold in cm to acquire target
#define LOST_DIST_CM        40.0    ///< Distance threshold in cm to lose target

// ==================== E-Stop Configuration ====================
#define DEBOUNCE_MS         50      ///< Minimum ms between valid button interrupt events

// ==================== Task Frequency Configuration ====================
#define SENSOR_TASK_HZ      128                        ///< Sensor polling rate in Hz
#define SENSOR_TASK_MS      (1000 / SENSOR_TASK_HZ)   ///< Period in ms (~8ms)
#define ESTOP_TASK_HZ       50                         ///< E-stop polling rate in Hz
#define ESTOP_TASK_MS       (1000 / ESTOP_TASK_HZ)    ///< Period in ms (20ms)

// ==================== System State ====================
/**
 * @brief Represents the three operating modes of the sentry system.
 * @details SCANNING: motors actively sweeping the dome.
 *          LOCKED:   target detected within TARGET_DIST_CM, motors frozen.
 *          ESTOP:    emergency stop pressed, servo homed, stepper released.
 */
enum SystemState { SCANNING, LOCKED, ESTOP };

// ==================== Shared Volatile State ====================
volatile SystemState  systemState       = SCANNING; ///< Current operating mode
volatile bool         estopActive       = false;    ///< Set by estopISR on button press
volatile unsigned long lastDebounceTime = 0;        ///< Tracks last valid button event
volatile bool         buzzEnabled       = false;    ///< Enables buzzer timer toggling
volatile uint32_t     buzzTickCount     = 0;        ///< Counts buzzer ISR firings
volatile float        stepperAngle      = 0.0;      ///< Current stepper angle in degrees
volatile int          servoAngleDeg     = SERVO_START_DEG; ///< Current servo tilt in degrees

// ==================== Hardware Timer ====================
hw_timer_t *buzzTimer = NULL; ///< Timer 0 handle for buzzer toggle ISR

// ==================== FreeRTOS Primitives ====================
QueueHandle_t     servoQueue = NULL; ///< Servo angle commands (scanTask -> servoWriteTask)
QueueHandle_t     distQueue  = NULL; ///< Latest distance reading (sensorTask -> lcdTask)
SemaphoreHandle_t stateMutex = NULL; ///< Protects systemState, stepperAngle, servoAngleDeg

// ==================== Hardware Objects ====================
Servo             myServo;                          ///< Servo motor object
Stepper           myStepper(2048, IN1, IN3, IN2, IN4); ///< Stepper motor object
LiquidCrystal_I2C lcd(0x27, 16, 2);                ///< 16x2 I2C LCD object

// ==================== ISR Implementations ====================
/**
 * @brief Hardware timer ISR that drives the buzzer on/off pattern.
 * @details Fires every 500ms via Timer 0. Toggles the buzzer PWM output while
 *          buzzEnabled is true. Automatically silences after BUZZ_DURATION_TICKS
 *          firings (2 seconds) by clearing buzzEnabled. Runs from IRAM for
 *          deterministic low-latency execution.
 */
void IRAM_ATTR buzzTimerISR() {
  if (!buzzEnabled) {
    ledcWrite(buzzerPin, 0);
    return;
  }

  buzzTickCount++;
  if (buzzTickCount > BUZZ_DURATION_TICKS) {
    buzzEnabled   = false;
    buzzTickCount = 0;
    ledcWrite(buzzerPin, 0);
    return;
  }

  static bool buzzState = false;
  buzzState = !buzzState;
  ledcWrite(buzzerPin, buzzState ? BUZZ_DUTY_ON : 0);
}

/**
 * @brief GPIO interrupt handler for the emergency stop button.
 * @details Fires on both RISING and FALLING edges (CHANGE mode) to catch
 *          both press and release events. Time-based debouncing discards
 *          events within DEBOUNCE_MS of the last valid event. Only sets or
 *          clears the estopActive flag so no heavy work is performed in ISR.
 */
void IRAM_ATTR estopISR() {
  unsigned long now = millis();
  if (now - lastDebounceTime < DEBOUNCE_MS) return;
  lastDebounceTime = now;
  estopActive = (digitalRead(estopPin) == LOW);
}

// ==================== Servo Helpers ====================
/**
 * @brief Write a logical servo angle, applying the physical offset.
 * @details Posts the physical angle to servoQueue. The servoWriteTask on
 *          Core 0 executes the actual myServo.write() call to avoid
 *          ESP32Servo cross-core threading issues.
 * @param logicalDeg Desired angle in logical degrees (0 = horizontal, 70 = max up).
 */
void servoWrite(int logicalDeg) {
  int physical = constrain(logicalDeg + SERVO_PHYSICAL_OFFSET, 0, 180);
  xQueueSend(servoQueue, &physical, portMAX_DELAY);
}

// ==================== Stepper Helpers ====================
/**
 * @brief Release all stepper motor coils to prevent heat buildup.
 * @details Sets IN1 through IN4 LOW. Should be called after any stepper
 *          movement completes or when the motor needs to be idle.
 */
void releaseStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

/**
 * @brief Move the stepper motor by a given number of degrees.
 * @details Steps one increment at a time, checking hard angle limits and
 *          system state on every step. Aborts early and releases coils if
 *          LOCKED or ESTOP is detected mid-move, or if STEPPER_MAX_DEG
 *          would be exceeded in either direction.
 * @param degrees Degrees to move. Positive = clockwise, negative = counter-clockwise.
 * @return true if the full move completed, false if aborted early.
 */
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

    // Abort mid-move if state changes
    if (systemState == LOCKED || systemState == ESTOP) {
      releaseStepper();
      return false;
    }
  }

  releaseStepper();
  return true;
}

/**
 * @brief Drive the stepper motor back to 0 degrees (center position).
 * @details Reads the current stepperAngle and calls stepDegrees() with its
 *          inverse to return to center. Resets stepperAngle to 0.0 and
 *          releases coils on completion.
 */
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

// ==================== Ultrasonic Helpers ====================
/**
 * @brief Take a single ultrasonic distance reading.
 * @details Sends a 10us trigger pulse on trigPin and measures the echo
 *          duration on echoPin. Uses a 30ms timeout to avoid blocking
 *          indefinitely if no echo is received.
 * @return Distance in centimeters, or -1.0 on timeout/no echo.
 */
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

/**
 * @brief Take an averaged ultrasonic distance reading for stability.
 * @details Calls getDistance() three times with 10ms gaps between readings
 *          and returns the mean of all valid (positive) results. Reduces
 *          the effect of spurious sensor noise on target detection.
 * @return Averaged distance in centimeters, or -1.0 if all readings failed.
 */
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

#endif // HELPERS_H
