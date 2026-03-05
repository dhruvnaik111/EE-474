/**
 * @file Lab4Part2.ino
 * @authors Dhruv Naik, Ethan Le
 * @date 02/28/2026
 * @brief Dual-Core Light Sensor and Anomaly Detection System
 * @details Implements a real-time system using FreeRTOS that runs on both cores
 * of an ESP32 to monitor light levels, detect anomalies, and perform calculations.
 */

// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================
#define PHOTORESISTOR_PIN 1
#define LED_PIN 2
#define WINDOW_SIZE 5

// ==================== Global Variables ====================
volatile double smaValue = 0.0;
volatile double currentLightLevel = 0.0;

SemaphoreHandle_t xBinarySemaphore;
TaskHandle_t lightDetectorTaskHandle;
TaskHandle_t lcdTaskHandle;
TaskHandle_t anomalyAlarmTaskHandle;
TaskHandle_t primeCalculationTaskHandle;

/**
 * @struct SimpleMovingAverage
 * @brief Structure to maintain simple moving average calculation state
 */
typedef struct {
    /** @brief Buffer to store the last N light sensor values */
    double values[WINDOW_SIZE];
    /** @brief Sum of all values currently in the buffer */
    double sum;
    /** @brief Count of values in the buffer (max is WINDOW_SIZE) */
    int count;
    /** @brief Current index position in the circular buffer */
    int index;
} SimpleMovingAverage;

// ==================== Hardware Objects ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================== Function Prototypes ====================
void lightDetectorTask(void *arg);
void lcdTask(void *arg);
void anomalyAlarmTask(void *arg);
void primeCalculationTask(void *arg);
bool isPrime(int num);

// ==================== Function Implementations ====================
void setup() {
   //         1. Initialize pins, serial, LCD, etc
   Serial.begin(9600);
   Wire.begin(8,9);
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();
   pinMode(PHOTORESISTOR_PIN, INPUT);
   pinMode(LED_PIN, OUTPUT);

   //         2. Create binary semaphore for synchronization of light level data.
   xBinarySemaphore = xSemaphoreCreateBinary();
   if (xBinarySemaphore != NULL) {
      // Initialize the semaphore as available
      xSemaphoreGive(xBinarySemaphore);

      //         3. Create Tasks
      //          - Create the `Light Detector Task` and assign it to Core 0.
      xTaskCreatePinnedToCore(lightDetectorTask, "lightDetectorTask", 2048, NULL, 1, &lightDetectorTaskHandle, 0);
      
      //          - Create `LCD Task` and assign it to Core 0.
      xTaskCreatePinnedToCore(lcdTask, "lcdTask", 2048, NULL, 1, &lcdTaskHandle, 0);
      
      //          - Create `Anomaly Alarm Task` and assign it to Core 1.
      xTaskCreatePinnedToCore(anomalyAlarmTask, "anomalyAlarmTask", 2048, NULL, 1, &anomalyAlarmTaskHandle, 1);
      
      //          - Create `Prime Calculation Task` and assign it to Core 1.
      xTaskCreatePinnedToCore(primeCalculationTask, "primeCalculationTask", 2048, NULL, 1, &primeCalculationTaskHandle, 1);
   }
}

void loop() {}

/**
 * @brief Continuously read light levels and calculate SMA
 * @details Reads analog values from the photoresistor, calculates a simple moving
 * average (SMA), and uses a binary semaphore to signal when new data is ready.
 * Executes on Core 0. Runs indefinitely with 100ms delay between readings.
 * @param arg Unused task parameter (pointer to void)
 */
void lightDetectorTask(void *arg) {
   // ====================> TODO:
   //          1. Initialize Variables
   SimpleMovingAverage sma = { .sum = 0.0, .count = 0, .index = 0 };
   for (int i = 0; i < WINDOW_SIZE; i++) {
      sma.values[i] = 0.0;
   }
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

      // Delay for 0.1 seconds before next reading
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Calculate the Simple Moving Average with a new value
 * @details Updates the circular buffer with a new value, maintains the running sum,
 * and returns the current average of all values in the buffer.
 * @param sma Pointer to the SimpleMovingAverage structure to update
 * @param newValue The new sensor reading to add to the SMA calculation
 * @return The current simple moving average value
 */
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

/**
 * @brief Update LCD display with current light level and SMA values
 * @details Waits for the binary semaphore, then updates the LCD display only when
 * values have changed. Displays current light level on line 1 and SMA on line 2.
 * Executes on Core 0 with 100ms polling interval.
 * @param arg Unused task parameter (pointer to void)
 */
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
            // Note: Removed lcd.clear() to fix screen flickering
            lcd.setCursor(0, 0);
            lcd.print("Light: ");
            lcd.print(currentLightLevel);
            lcd.print("    "); // Pad with spaces to overwrite old trailing digits
            
            lcd.setCursor(0, 1);
            lcd.print("SMA: ");
            lcd.print(smaValue);
            lcd.print("    "); // Pad with spaces to overwrite old trailing digits
            
            oldSMA = smaValue;
            oldLightLevel = currentLightLevel;
         }
         //            - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }

      // Delay for 0.1 seconds before checking again
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Monitor SMA values and trigger LED alarm on anomaly detection
 * @details Continuously monitors the SMA of light levels and flashes an LED signal
 * if an anomaly is detected (SMA > 3800 or SMA < 300). Performs 3 flash cycles
 * (100ms on, 2000ms off) per anomaly. Executes on Core 1.
 * @param arg Unused task parameter (pointer to void)
 */
void anomalyAlarmTask(void *arg) {
   // ====================> TODO:
   //            1. Loop Continuously
   while(1) {
      double localSma = 0.0;
      
      //             - Wait for semaphore.
      if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
         // Safely copy the SMA value and immediately release the semaphore to prevent blocking
         localSma = smaValue;
         
         //             - Give back the semaphore.
         xSemaphoreGive(xBinarySemaphore);
      }
      
      //             - Check if SMA indicates a light anomaly (SMA > 3800 or SMA <300).
      if(localSma > 3800 || localSma < 300) {
         //             - If an anomaly is detected, flash a LED signal
         for(int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
         }
      }
      
      // Delay for 0.1 seconds before checking again
      vTaskDelay(100 / portTICK_PERIOD_MS);
   }
}

/**
 * @brief Calculate and print prime numbers to serial monitor
 * @details Iterates from 2 to 5000, checking each number for primality and printing
 * prime numbers to the serial monitor. Executes on Core 1 and deletes itself
 * after completion. Used to demonstrate dual-core task scheduling.
 * @param arg Unused task parameter (pointer to void)
 */
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
   
   // Clean up the task once the limit of 5000 is reached
   vTaskDelete(NULL); 
}

/**
 * @brief Check if a number is prime
 * @details Uses trial division method: tests divisibility up to sqrt(num).
 * @param num The number to check for primality
 * @return true if num is prime, false otherwise
 */
bool isPrime(int num) {
   for (int i = 2; i <= sqrt(num); i++) {
      if (num % i == 0) return false;
   }
   return true;
}