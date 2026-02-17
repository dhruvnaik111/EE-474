// Filename: Lab4Part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/21/2026
// Description: Implement a Dual-Core Light Sensor and Anomaly Detection System
// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>

// ==================== Global Variables ====================
TaskHandle_t lightDetectorTaskHandle;
TaskHandle_t lcdTaskHandle;
TaskHandle_t anomalyAlarmTaskHandle;
TaskHandle_t primeCalculationTaskHandle;
// ==================== Macros ====================
#define PHOTORESISTOR_PIN 1

//Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

setup() {
   //         1. Initialize pins, serial, LCD, etc
   Serial.begin(9600);
   Wire.begin(8,9);
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();
   //         2. Create binary semaphore for synchronization of light level data.
   //         3. Create Tasks
   //          - Create the `Light Detector Task` and assign it to Core 0.
   xTaskCreatePinnedToCore(lightDetectorTask, "lightDetectorTask", 1024, NULL, 1, &lightDetectorTaskHandle, 0);
   //          - Create `LCD Task` and assign it to Core 0.
   xTaskCreatePinnedToCore(lcdTask, "lcdTask", 1024, NULL, 1, &lcdTaskHandle, 0);
   //          - Create `Anomaly Alarm Task` and assign it to Core 1.
   xTaskCreatePinnedToCore(anomalyAlarmTask, "anomalyAlarmTask", 1024, NULL, 1, &anomalyAlarmTaskHandle, 1);
   //          - Create `Prime Calculation Task` and assign it to Core 1.
   xTaskCreatePinnedToCore(primeCalculationTask, "primeCalculationTask", 1024, NULL, 1, &primeCalculationTaskHandle, 1);
}
loop() {}


// Name: lightDetectorTask
// Description: Continuously read light levels from the photoresistor, calculate a simple moving average (SMA), and signal when new data is ready (Core 0)
void lightDetectorTask(void *arg) {
// ====================> TODO:
//          1. Initialize Variables
//          2. Loop Continuously
while (1) {
   //           - Read light level from the photoresistor.
   int lightLevel = analogRead(PHOTORESISTOR_PIN);
   //           - Take semaphore
   //           - Calculate the simple moving average and update variables.
   //           - Give semaphore to signal data is ready.
}

}



// Name: lcdTask
// Description: Wait for light level data to be ready, then update the LCD with the current light level and SMA. (Core 0)
void lcdTask(void *arg) {
// ====================> TODO:
//          1. Initialize Variables
//           2. Loop Continuously
//            - Wait for semaphore.
//            - If data has changed, update the LCD with the new light level and SMA.
//            - Give back the semaphore.
}



// Name: anomalyAlarmTask
// Description: Monitor the SMA of light levels and flash an LED if an anomaly is detected (Core 1)
void anomalyAlarmTask(void *arg) {
// ====================> TODO:
//            1. Loop Continuously
//             - Wait for semaphore.
//             - Check if SMA indicates a light anomaly (outside thresholds).
//             - If an anomaly is detected, flash a LED signal.
//             - Give back the semaphore.
}


//Name: primeCalculationTask
//Description: Continuously calculate prime numbers and print them to the serial monitor. (Core 1)
void primeCalculationTask(void *arg) {
   // ====================> TODO:
   //            1. Loop from 2 to 5000
   for(int i = 2; i <= 5000; i++) {
      //             - Check if the current number is prime.
      //             - If prime, print the number to the serial monitor.
      if (isPrime(i)) {
         Serial.print(i);
         Serial.print(" ");
      }
   }
}

// Name: isPrime
// Description: Helper function to check if a number is prime
bool isPrime(int num) {
   for (int i = 2; i <= sqrt(num); i++) {
      if (num % i == 0) return false;
   }
   return true;
}

