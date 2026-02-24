// Filename: Lab4Part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/28/2026
// Description: Implement a Dual-Core Light Sensor and Anomaly Detection System
// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>

// ==================== Global Variables ====================
TaskHandle_t lightDetectorTaskHandle;
TaskHandle_t lcdTaskHandle;
TaskHandle_t anomalyAlarmTaskHandle;
TaskHandle_t primeCalculationTaskHandle;

typedef struct {
    double values[WINDOW_SIZE]; // Buffer to store the last N values
    double sum;
    int count;
    int index;
} SimpleMovingAverage;

volatile double smaValue = 0.0;
volatile double currentLightLevel = 0.0;
// ==================== Macros ====================
#define PHOTORESISTOR_PIN 1
#define WINDOW_SIZE 5



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
   xBinarySemaphore = xSemaphoreCreateBinary();
   if(xBinarySemaphore != NULL){
      xTaskCreatePinnedToCore(lightDetectorTask, "lightDetectorTask", 1024, NULL, 1, &lightDetectorTaskHandle, 0);
   }
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
   SimpleMovingAverage sma = { .sum = 0.0, .count = 0, .index = 0 };
   currentLightLevel = 0.0;
   smaValue = 0.0;
   //          2. Loop Continuously
   while (1) {
      //           - Read light level from the photoresistor.
      currentLightLevel = analogRead(PHOTORESISTOR_PIN);
      //           - Take semaphore
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         //           - Calculate the simple moving average and update variables.
         smaValue = calculateSMA(&sma, currentLightLevel);
         //           - Give semaphore to signal data is ready.
         xSemaphoreGive(xBinarySemaphore);
      }

   }
}

// Name: calculateSMA
// Description: Helper function to calculate the Simple Moving Average (SMA) given a new value
double calculateSMA(SimpleMovingAverage *sma, double newValue) {
   // Remove the oldest value from the sum
   sma->sum -= sma->values[sma->index];
   // Add the new value to the buffer and sum
   sma->values[sma->index] = newValue;
   sma->sum += newValue;
   // Move index forward and wrap around if necessary
   sma->index = (sma->index + 1) % WINDOW_SIZE;
   // Update count of values (max is WINDOW_SIZE)
   if (sma->count < WINDOW_SIZE) {
      sma->count++;
   }
   // Return the current SMA
   return sma->sum / sma->count;
}



// Name: lcdTask
// Description: Wait for light level data to be ready, then update the LCD with the current light level and SMA. (Core 0)
void lcdTask(void *arg) {
   // ====================> TODO:
   //          1. Initialize Variables
   double oldSMA = 0.0;
   double oldLightLevel = 0.0;
   //           2. Loop Continuously
   while (1) {
      //            - Wait for semaphore.
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         //            - If data has changed, update the LCD with the new light level and SMA.
         if(smaValue != oldSMA || currentLightLevel != oldLightLevel) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Light: ");
            lcd.print(currentLightLevel);
            lcd.setCursor(0, 1);
            lcd.print("SMA: ");
            lcd.print(smaValue);
            oldSMA = smaValue;
            oldLightLevel = currentLightLevel;
         }
         //            - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }

   }
}



// Name: anomalyAlarmTask
// Description: Monitor the SMA of light levels and flash an LED if an anomaly is detected (Core 1)
void anomalyAlarmTask(void *arg) {
   // ====================> TODO:
   //            1. Loop Continuously
   while(1) {
      //             - Wait for semaphore.
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         //             - Check if SMA indicates a light anomaly (SMA > 3800 or SMA <300).
         if(smaValue > 3800 || smaValue < 300) {
            //             - If an anomaly is detected, flash a LED signal
            for(int i = 0; i < 3; i++) {
               digitalWrite(LED_BUILTIN, HIGH);
               vTaskDelay(2000 / portTICK_PERIOD_MS);
               digitalWrite(LED_BUILTIN, LOW);
               vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
         }
         //             - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }

   }
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

