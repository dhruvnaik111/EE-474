// Filename: Lab4Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/28/2026
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
      // 1. Do the work ONCE per period
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));

      // 2. Countdown the remaining time in chunks (Allows scheduler to preempt)
      while (remainingLedTime > 0) {
         vTaskDelay(250 / portTICK_PERIOD_MS); 
         remainingLedTime -= 250 / portTICK_PERIOD_MS; 
         vTaskSuspend(NULL); 
      }

      // 3. Reset time for the next cycle
      remainingLedTime = ledTaskExecutionTime;
   }
}

// Name: counterTask
// Description: Print out an incrementing counter to your LCD, and update remaining time for this task
void counterTask(void *arg) {
   static int counter = 0;
   while (1) {
      // 1. Do the work ONCE per period
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Count: ");
      lcd.print(counter++);

      // 2. Countdown the remaining time in chunks (Allows scheduler to preempt)
      while (remainingCounterTime > 0) {
         vTaskDelay(100 / portTICK_PERIOD_MS); 
         remainingCounterTime -= 100 / portTICK_PERIOD_MS; 
         vTaskSuspend(NULL);
      }

      // 3. Reset time for the next cycle (Do NOT reset the 'counter' variable here!)
      remainingCounterTime = counterTaskExecutionTime;
   }
}

// Name: alphabetTask
// Description: Print out the alphabet to Serial, and update remaining time for this task
void alphabetTask(void *arg) {
   char letter = 'A'; 

   while(1) {
      // 1. Do the work ONCE per period
      Serial.print(letter++);
      Serial.print(" ");
      
      if (letter > 'Z') {
         letter = 'A';
      }

      // 2. Countdown the remaining time in chunks (Allows scheduler to preempt)
      while (remainingAlphabetTime > 0) {
         vTaskDelay(500 / portTICK_PERIOD_MS);
         remainingAlphabetTime -= 500 / portTICK_PERIOD_MS;
         vTaskSuspend(NULL);
      }

      // 3. Reset time for the next cycle
      remainingAlphabetTime = alphabetTaskExecutionTime;
   }
}

// Name: scheduleTasks
// Description: Implement Shortest Remaining Time First scheduling logic. This function should select the task with
//              the shortest remaining time and run it. Once a task completes it should
//              reset its remaining time.
// Name: scheduleTasks
// Description: Implement Shortest Remaining Time First scheduling logic.
void scheduleTasks(void *arg) {
   while (1) {
      TickType_t shortestTime = 0xFFFFFFFF; // Start with the maximum possible value
      TaskHandle_t taskToResume = NULL;

      // 1. Check if LED Task is waiting (suspended) AND has the shortest time so far
      if (eTaskGetState(ledTaskHandle) == eSuspended && remainingLedTime <= shortestTime) {
         shortestTime = remainingLedTime;
         taskToResume = ledTaskHandle;
      }

      // 2. Check if Counter Task is waiting (suspended) AND has the shortest time so far
      if (eTaskGetState(counterTaskHandle) == eSuspended && remainingCounterTime <= shortestTime) {
         shortestTime = remainingCounterTime;
         taskToResume = counterTaskHandle;
      }

      // 3. Check if Alphabet Task is waiting (suspended) AND has the shortest time so far
      if (eTaskGetState(alphabetTaskHandle) == eSuspended && remainingAlphabetTime <= shortestTime) {
         shortestTime = remainingAlphabetTime;
         taskToResume = alphabetTaskHandle;
      }

      // 4. If we found a suspended task, resume the winner!
      if (taskToResume != NULL) {
         vTaskResume(taskToResume);
      }

      // Delay to let the worker tasks run before we schedule again
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
   // 1. Create the worker tasks first (Priority 1)
   xTaskCreatePinnedToCore(ledTask, "ledTask", 4096, NULL, 1, &ledTaskHandle, 0);
   xTaskCreatePinnedToCore(counterTask, "counterTask", 4096, NULL, 1, &counterTaskHandle, 0);
   xTaskCreatePinnedToCore(alphabetTask, "alphabetTask", 4096, NULL, 1, &alphabetTaskHandle, 0);

   // 2. Create the scheduler task LAST (Priority 2)
   xTaskCreatePinnedToCore(scheduleTasks, "scheduleTasks", 4096, NULL, 2, &schedulerTaskHandle, 0);

   //initialize LED pin
   pinMode(LED_PIN, OUTPUT);

}
void loop() {}