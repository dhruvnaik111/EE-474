// Filename: icte8.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/26/2026
// Description: Demonstrates task notifications in FreeRTOS with a simple LED blinking example

// ==================== Includes ====================
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==============================================
// Pin Definitions
// ==============================================
#define LED_PIN 5   // TODO: connect LED + resistor to GND

// ==============================================
// Task Handle(s)
// ==============================================
// TODO: Declare a global TaskHandle_t for the receiving task (BlinkTask)
// so the sender can notify it.
TaskHandle_t blinkTaskHandle;
TaskHandle_t notifierTaskHandle;


// ==============================================
// Task A: BlinkTask (Receiver)
// ==============================================
// TODO:
//  - Configure LED pin as OUTPUT once.
//  - Block with ulTaskNotifyTake(pdTRUE, portMAX_DELAY).
//  - When a notification is received, toggle LED once.
//  - Then loop back and wait again.
void BlinkTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}


// ==============================================
// Task B: NotifierTask (Sender)
// ==============================================
// TODO:
//  - Every 1000 ms, call xTaskNotifyGive(<blink task handle>).
//  - Use vTaskDelay(pdMS_TO_TICKS(1000)) for timing (no delay()).
//  - Receives the latest period from Task C updates its local rate.
//  - Then drives Task A at the new rate.
void NotifierTask(void *pvParameters) {
  uint32_t period = 1000; // Default period in ms
  uint32_t notificationValue = 0;
  
  while (1) {
    // Check if a new period is available from ConfigTask
    notificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
    if(notificationValue > 0) {
      // Update the period with the new value received from ConfigTask  
      period = notificationValue;
    }
    // Notify the BlinkTask to toggle the LED at the current rate
    xTaskNotifyGive(blinkTaskHandle);

    // Wait for the configured period
    vTaskDelay(pdMS_TO_TICKS(period));
  }
}


// ==============================================
// Task C: ConfigTask (Producer of period values)
// ==============================================
// TODO:
//  - Every 5 seconds, generate a random period in the range 0.5–1.5 seconds (i.e., 500–1500 ms).
//  - Send this period to Task B via task notification with a value
void ConfigTask(void *pvParameters) {
  while (1) {
    // Generate a random period between 500 and 1500 ms
    uint32_t randomPeriod = random(500, 1501);

    // Send this period to Task B via task notification with a value
    xTaskNotify(notifierTaskHandle, randomPeriod, eSetValueWithOverwrite);
    Serial.print("New period set to ");
    Serial.println(randomPeriod);

    // Wait for 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// ==============================================
// Setup
// ==============================================
void setup() {
  Serial.begin(115200);

  // TODO: Create BlinkTask FIRST and capture its handle.
  xTaskCreate(BlinkTask, "BlinkTask", 1024, NULL, 1, &blinkTaskHandle);

  // TODO: Create NotifierTask SECOND.
  xTaskCreate(NotifierTask, "NotifierTask", 1024, NULL, 1, &notifierTaskHandle);

  // TODO: Create ConfigTask THIRD.
  xTaskCreate(ConfigTask, "ConfigTask", 1024, NULL, 1, NULL);

  // (Optional) brief print that the demo started.
  Serial.println("Task Notification demo started");
}


// ==============================================
// Loop
// ==============================================
void loop() {
  // Leave empty. FreeRTOS handles scheduling.
}