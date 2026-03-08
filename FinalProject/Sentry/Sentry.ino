/**
 * @file SentryProject.ino
 * @authors Dhruv Naik, Ethan Le
 * @date 03/07/2026
 * @brief Dual-Core Autonomous Tracking Sentry
 * @details Implements a real-time system using FreeRTOS that runs on both cores
 * of an ESP32 to monitor acoustic/ultrasonic sensors, update an LCD UI, 
 * and perform PID motor calculations.
 */

// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Stepper.h>

// ==================== Macros ====================
#define SERVO_PIN 13
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17
#define STOP_BUTTON_PIN 27
#define STATUS_LED_PIN 14

// ==================== Global Variables ====================
// Target tracking globals replacing FreeRTOS Queues to match lab structure
volatile float globalDistance = 0.0;
volatile float globalAngle = 0.0;
volatile bool globalTargetAcquired = false;
volatile bool emergencyStop = false;

volatile int currentServoPos = 90;

SemaphoreHandle_t xBinarySemaphore;
TaskHandle_t sensorTaskHandle;
TaskHandle_t lcdTaskHandle;
TaskHandle_t actuatorTaskHandle;

// ==================== Hardware Objects ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo tiltServo;
const int stepsPerRev = 2048; 
Stepper panStepper(stepsPerRev, IN1, IN3, IN2, IN4);

// ==================== Function Prototypes ====================
void sensorTask(void *arg);
void lcdTask(void *arg);
void actuatorTask(void *arg);
void IRAM_ATTR handleEmergencyStop();
void moveServoGentle(int target);
void releaseStepper();

// ==================== Function Implementations ====================
void setup() {
   //         1. Initialize pins, serial, LCD, etc
   Serial.begin(115200);
   Wire.begin(8,9); // Custom I2C pins from lab implementation
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();
   
   pinMode(STATUS_LED_PIN, OUTPUT);
   pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(STOP_BUTTON_PIN), handleEmergencyStop, FALLING);

   // Initialize Motors
   ESP32PWM::allocateTimer(0);
   tiltServo.setPeriodHertz(50);
   tiltServo.attach(SERVO_PIN, 500, 2400);
   tiltServo.write(currentServoPos);
   panStepper.setSpeed(10);
   releaseStepper();

   //         2. Create binary semaphore for synchronization of target data.
   xBinarySemaphore = xSemaphoreCreateBinary();
   if (xBinarySemaphore != NULL) {
      // Initialize the semaphore as available
      xSemaphoreGive(xBinarySemaphore);

      //         3. Create Tasks
      //          - Create the `Sensor Task` and assign it to Core 0.
      xTaskCreatePinnedToCore(sensorTask, "sensorTask", 4096, NULL, 1, &sensorTaskHandle, 0);

      //          - Create `LCD Task` and assign it to Core 0.
      xTaskCreatePinnedToCore(lcdTask, "lcdTask", 2048, NULL, 1, &lcdTaskHandle, 0);

      //          - Create `Actuator Task` and assign it to Core 1.
      xTaskCreatePinnedToCore(actuatorTask, "actuatorTask", 4096, NULL, 1, &actuatorTaskHandle, 1);
   }
}

void loop() {}

// --- Interrupt Service Routine (ISR) ---
void IRAM_ATTR handleEmergencyStop() {
  emergencyStop = !emergencyStop; 
}

/**
 * @brief Continuously read sensors and update tracking globals
 * @details Reads acoustic and distance values, and uses a binary semaphore 
 * to signal when new telemetry data is ready.
 * Executes on Core 0. Runs indefinitely with 10ms delay between readings.
 * @param arg Unused task parameter (pointer to void)
 */
void sensorTask(void *arg) {
   // ====================> TODO:
   //          1. Initialize Variables
   float localDist = 0.0;
   float localAng = 0.0;
   bool localAcquired = false;

   //          2. Loop Continuously
   while (1) {
      if (!emergencyStop) {
         //           - Read sensor levels (Placeholders for HC-SR04 & INMP441)
         localDist = 150.0; 
         localAng = 45.0; 
         localAcquired = true;
         
         //           - Take semaphore
         if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            //           - Update variables.
            globalDistance = localDist;
            globalAngle = localAng;
            globalTargetAcquired = localAcquired;

            //           - Give semaphore to signal data is ready.
            xSemaphoreGive(xBinarySemaphore);
         }
      }

      // Delay for maintaining loop frequency
      vTaskDelay(10 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Update LCD display with current distance and angle values
 * @details Waits for the binary semaphore, then updates the LCD display only when
 * values have changed.
 * Displays current distance on line 1 and angle on line 2.
 * Executes on Core 0 with 100ms polling interval.
 * @param arg Unused task parameter (pointer to void)
 */
void lcdTask(void *arg) {
   // ====================> TODO:
   //          1. Initialize Variables
   float oldDistance = -1.0;
   float oldAngle = -1.0;

   //           2. Loop Continuously
   while (1) {
      //            - Wait for semaphore.
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         //            - If data has changed, update the LCD with new distance and angle.
         if(globalDistance != oldDistance || globalAngle != oldAngle) {
            // Note: Removed lcd.clear() to fix screen flickering
            lcd.setCursor(0, 0);
            lcd.print("Dist: ");
            lcd.print(globalDistance);
            lcd.print("    "); // Pad with spaces to overwrite old trailing digits
            
            lcd.setCursor(0, 1);
            lcd.print("Ang:  ");
            lcd.print(globalAngle);
            lcd.print("    "); // Pad with spaces to overwrite old trailing digits
            
            oldDistance = globalDistance;
            oldAngle = globalAngle;
         }
         //            - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }

      // Delay for 0.1 seconds before checking again
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Monitor target globals and execute PID motor control loop
 * @details Continuously reads tracking variables safely and drives Stepper 
 * and Servo motors for physical orientation.
 * Executes on Core 1.
 * @param arg Unused task parameter (pointer to void)
 */
void actuatorTask(void *arg) {
   // ====================> TODO:
   //            1. Loop Continuously
   while(1) {
      float localDist = 0.0;
      float localAng = 0.0;
      bool localAcquired = false;

      if (emergencyStop) {
         digitalWrite(STATUS_LED_PIN, HIGH);
         releaseStepper();
         vTaskDelay(100 / portTICK_PERIOD_MS);
         continue;
      }
      
      digitalWrite(STATUS_LED_PIN, LOW);

      //             - Wait for semaphore.
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         // Safely copy the target values and immediately release the semaphore to prevent blocking
         localDist = globalDistance;
         localAng = globalAngle;
         localAcquired = globalTargetAcquired;
         //             - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }
      
      //             - Check if target acquired and execute PID
      if(localAcquired) {
         // Move Motors Safely (Placeholder PID logic)
         if (localAng > 0) {
            panStepper.step(10);
            releaseStepper(); 
         }

         if (localDist < 100) {
            moveServoGentle(110);
         }
      }
      
      // Delay for ~64Hz operating frequency
      vTaskDelay(15 / portTICK_PERIOD_MS);
   }
}

// ==================== Power-Saving Motor Helpers ====================
/**
 * @brief Move servo gradually to prevent power brownouts
 * @param target The target angle (0-180)
 */
void moveServoGentle(int target) {
  int stepDir = (target > currentServoPos) ? 1 : -1;
  while (currentServoPos != target) {
    currentServoPos += stepDir;
    tiltServo.write(currentServoPos);
    vTaskDelay(15 / portTICK_PERIOD_MS); 
  }
}

/**
 * @brief De-energize stepper coils to prevent regulator overheating
 */
void releaseStepper() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
}