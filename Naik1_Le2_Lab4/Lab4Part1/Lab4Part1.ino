// Filename: Lab4Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/21/2026
// Description: Manage and schedule tasks with preemptive scheduling algorithms in freeRTOS
// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>

// ==================== Global Variables ====================

// Total times for tasks
const TickType_t ledTaskExecutionTime = 500 / portTICK_PERIOD_MS;      // 500 ms
const TickType_t counterTaskExecutionTime = 2000 / portTICK_PERIOD_MS; // 2 seconds
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS; // 13 seconds
// Remaining Execution Times
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

// Task Handles
TaskHandle_t ledTaskHandle;
TaskHandle_t counterTaskHandle;
TaskHandle_t alphabetTaskHandle;
TaskHandle_t schedulerTaskHandle;

//Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LED_PIN 2

// Name: ledTask
// Description: Blink an LED and update remaining time for this task
void ledTask(void *arg) {
   while (1) {
      if(remainingLedTime <= 0) {
         // Reset remaining time for next round
         remainingLedTime = ledTaskExecutionTime;
      } else {
         digitalWrite(LED_PIN, !digitalRead(LED_PIN));
         vTaskDelay(250 / portTICK_PERIOD_MS); // Blink every 250 ms
         remainingLedTime -= 250 / portTICK_PERIOD_MS; // Decrease remaining time
      }
   }
}

// Name: counterTask
// Description: Print out an incrementing counter to your LCD, and update remaining time for this task
void counterTask(void *arg) {
   static int count = 1;

   while(1){
      if (remainingCounterTime <= 0) {
         count = 1;
         remainingCounterTime = counterTaskExecutionTime;
      } else {
         lcd.clear();
         lcd.setCursor(0, 0);
         static int counter = 0;
         lcd.print("Count: ");
         lcd.print(counter++);
         vTaskDelay(100 / portTICK_PERIOD_MS); 
         remainingCounterTime -= 100 / portTICK_PERIOD_MS; 
      }
   }
}

// Name: alphabetTask
// Description: Print out the alphabet to Serial, and update remaining time for this task
void alphabetTask(void *arg) {
   char letter = 'A';

   while(1) {
      if (remainingAlphabetTime <= 0) {
         remainingAlphabetTime = alphabetTaskExecutionTime;
      } else {
         Serial.print(letter++);
         Serial.print(" ");

         if (letter > 'Z') {
            letter = 'A';
         }
         vTaskDelay(500 / portTICK_PERIOD_MS);
         remainingAlphabetTime -= 500 / portTICK_PERIOD_MS;
      }
   }
}

// Name: scheduleTasks
// Description: Implement Shortest Remaining Time First scheduling logic. This function should select the task with
//              the shortest remaining time and run it. Once a task completes it should
//              reset its remaining time.
void scheduleTasks(void *arg) {
   while (1) {
      if(remainingLedTime < remainingCounterTime && remainingLedTime < remainingAlphabetTime) {
         vTaskResume(ledTaskHandle);
         vTaskSuspend(counterTaskHandle);
         vTaskSuspend(alphabetTaskHandle);
      } else if(remainingCounterTime < remainingLedTime && remainingCounterTime < remainingAlphabetTime) {
         vTaskResume(counterTaskHandle);
         vTaskSuspend(ledTaskHandle);
         vTaskSuspend(alphabetTaskHandle);
      } else if(remainingAlphabetTime < remainingLedTime && remainingAlphabetTime < remainingCounterTime) {
         vTaskResume(alphabetTaskHandle);
         vTaskSuspend(ledTaskHandle);
         vTaskSuspend(counterTaskHandle);
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
   }
}


void setup() {
   // TODO: Create 4 tasks and pin them to core 0:
   //          1. A scheduler that handles the scheduling of the other three tasks
   //          2. Blink an LED
   //          3. Print a counter to the LCD
   //          4. Print the alphabet to Serial

   //initialize serial monitor
   Serial.begin(9600);
   //initialize LCD
   Wire.begin(8,9);
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();

   //tasks
   xTaskCreatePinnedToCore(scheduleTasks, "scheduleTasks", 1024, NULL, 2, &scheduleTasksHandle, 0); //we want highest priority
   xTaskCreatePinnedToCore(ledTask, "ledTask", 1024, NULL, 1, &ledTaskHandle, 0);
   xTaskCreatePinnedToCore(counterTask, "counterTask", 1024, NULL, 1, &counterTaskHandle, 0);
   xTaskCreatePinnedToCore(alphabetTask, "alphabetTask", 1024, NULL, 1, &alphabetTaskHandle, 0);

   //initialize LED pin
   pinMode(LED_PIN, OUTPUT);

}
void loop() {}