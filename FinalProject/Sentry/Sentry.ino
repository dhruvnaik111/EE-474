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

// Ultrasonic Pins
#define TRIG_PIN 4
#define ECHO_PIN 2

// ==================== Global Variables ====================
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
float getDistance();

// ==================== Function Implementations ====================
void setup() {
   //         1. Initialize pins, serial, LCD, etc
   Serial.begin(115200);
   Wire.begin(8,9); 
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();
   
   pinMode(STATUS_LED_PIN, OUTPUT);
   pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(STOP_BUTTON_PIN), handleEmergencyStop, FALLING);

   // Initialize Ultrasonic Pins
   pinMode(TRIG_PIN, OUTPUT);
   pinMode(ECHO_PIN, INPUT);

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
      xSemaphoreGive(xBinarySemaphore);

      //         3. Create Tasks
      xTaskCreatePinnedToCore(sensorTask, "sensorTask", 4096, NULL, 1, &sensorTaskHandle, 0);
      xTaskCreatePinnedToCore(lcdTask, "lcdTask", 2048, NULL, 1, &lcdTaskHandle, 0);
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
         //           - Read sensor levels
         localDist = getDistance(); // Live Ultrasonic Data
         
         // Placeholder for INMP441 DSP logic
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
 * @details Waits for the binary semaphore, then updates the LCD display only when values have changed.
 * Executes on Core 0 with 100ms polling interval.
 * @param arg Unused task parameter (pointer to void)
 */
void lcdTask(void *arg) {
   float oldDistance = -1.0;
   float oldAngle = -1.0;

   while (1) {
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         if(globalDistance != oldDistance || globalAngle != oldAngle) {
            lcd.setCursor(0, 0);
            lcd.print("Dist: ");
            if (globalDistance < 0) lcd.print("ERR ");
            else lcd.print(globalDistance);
            lcd.print(" cm  "); 
            
            lcd.setCursor(0, 1);
            lcd.print("Ang:  ");
            lcd.print(globalAngle);
            lcd.print("    "); 
            
            oldDistance = globalDistance;
            oldAngle = globalAngle;
         }
         xSemaphoreGive(xBinarySemaphore);
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Monitor target globals and execute PID motor control loop
 * @details Continuously reads tracking variables safely and drives Stepper and Servo motors.
 * Executes on Core 1.
 * @param arg Unused task parameter (pointer to void)
 */
void actuatorTask(void *arg) {
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

      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         localDist = globalDistance;
         localAng = globalAngle;
         localAcquired = globalTargetAcquired;
         xSemaphoreGive(xBinarySemaphore);
      }
      
      if(localAcquired) {
         // Move Motors Safely (Placeholder PID logic)
         if (localAng > 0) {
            panStepper.step(10);
            releaseStepper(); 
         }

         if (localDist > 0 && localDist < 100) {
            moveServoGentle(110);
         }
      }
      
      vTaskDelay(15 / portTICK_PERIOD_MS);
   }
}

// ==================== Helper Functions ====================

/**
 * @brief Trigger HC-SR04 and calculate distance
 * @return Distance in cm, or -1.0 if out of range
 */
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 30ms timeout prevents the FreeRTOS task from hanging indefinitely
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
  
  if (duration == 0) {
    return -1.0; 
  }
  
  return (duration / 2.0) * 0.0343;
}

void moveServoGentle(int target) {
  int stepDir = (target > currentServoPos) ? 1 : -1;
  while (currentServoPos != target) {
    currentServoPos += stepDir;
    tiltServo.write(currentServoPos);
    vTaskDelay(15 / portTICK_PERIOD_MS); 
  }
}

void releaseStepper() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
}