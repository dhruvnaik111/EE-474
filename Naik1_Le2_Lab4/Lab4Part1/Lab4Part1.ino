// Filename: Lab4Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/28/2026
// Description: Manage and schedule tasks with preemptive scheduling algorithms in freeRTOS

// Version: 2.7
// Authors: Dhruv Naik, Ethan Le
// Date: 02/25/2026
// Change(s): Fixed comments to comply with the code guidelines. 


// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>


// ==================== Macros ====================
#define LED_PIN 2


// ==================== Global Constants ====================
// Execution times converted from milliseconds to FreeRTOS ticks
const TickType_t ledTaskExecutionTime = 500 / portTICK_PERIOD_MS;
const TickType_t counterTaskExecutionTime = 2000 / portTICK_PERIOD_MS;
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS;


// ==================== Global Variables ====================
// Remaining execution time for each task (shared between scheduler and tasks)
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

// Task handles used by the scheduler to resume suspended tasks
TaskHandle_t ledTaskHandle;
TaskHandle_t counterTaskHandle;
TaskHandle_t alphabetTaskHandle;
TaskHandle_t schedulerTaskHandle;


// ==================== Hardware Objects ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ==================== Function Prototypes ====================
void ledTask(void *arg);
void counterTask(void *arg);
void alphabetTask(void *arg);
void scheduleTasks(void *arg);


// ==================== Function Implementations ====================


// Name: ledTask
// Description: Toggles an LED when its execution period expires.
//              The task decreases its remaining time each slice
//              and suspends itself until resumed by the scheduler.
void ledTask(void *arg) {
   while (1) {

      // Execute when allocated execution time has expired
      if (remainingLedTime == 0) {
         remainingLedTime = ledTaskExecutionTime;
         Serial.println("LED Blinking...");
         digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }

      // Simulated execution slice of 250 ms
      vTaskDelay(250 / portTICK_PERIOD_MS);

      // Decrease remaining execution time
      remainingLedTime -= 250 / portTICK_PERIOD_MS;

      // Yield control so scheduler can select next task
      vTaskSuspend(NULL);
   }
}


// Name: counterTask
// Description: Displays an incrementing counter on the LCD every
//              2 seconds. The counter resets after reaching 20.
//              The task updates its remaining time before suspending.
void counterTask(void *arg) {
   static int counter = 1;

   while (1) {

      // Execute when allocated execution time has expired
      if (remainingCounterTime == 0) {
         remainingCounterTime = counterTaskExecutionTime;

         Serial.print("LCD Count: ");
         Serial.println(counter);

         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Count: ");
         lcd.print(counter);

         counter++;

         // Reset counter after reaching 20
         if (counter > 20) {
            counter = 0;
         }
      }

      vTaskDelay(100 / portTICK_PERIOD_MS);
      remainingCounterTime -= 100 / portTICK_PERIOD_MS;

      vTaskSuspend(NULL);
   }
}


// Name: alphabetTask
// Description: Prints letters A through Z to Serial every 13 seconds.
//              After reaching 'Z', the sequence restarts at 'A'.
//              The task updates its remaining time before suspending.
void alphabetTask(void *arg) {
   char letter = 'A';

   while (1) {

      // Execute when allocated execution time has expired
      if (remainingAlphabetTime == 0) {
         remainingAlphabetTime = alphabetTaskExecutionTime;

         Serial.print("Alphabet: ");
         Serial.println(letter);

         letter++;

         // Wrap back to 'A' after 'Z'
         if (letter > 'Z') {
            letter = 'A';
         }
      }

      vTaskDelay(500 / portTICK_PERIOD_MS);
      remainingAlphabetTime -= 500 / portTICK_PERIOD_MS;

      vTaskSuspend(NULL);
   }
}


// Name: scheduleTasks
// Description: Implements Shortest Remaining Time First scheduling
//              logic. The scheduler resumes the suspended task with
//              the smallest remaining execution time. Once a task
//              finishes, its remaining time is reset within the task.
void scheduleTasks(void *arg) {
   while (1) {

      // Start with maximum possible value for comparison
      TickType_t shortestTime = 0xFFFFFFFF;
      TaskHandle_t taskToResume = NULL;

      // Check each suspended task and select the one with smallest remaining time
      if (eTaskGetState(ledTaskHandle) == eSuspended &&
          remainingLedTime <= shortestTime) {
         shortestTime = remainingLedTime;
         taskToResume = ledTaskHandle;
      }

      if (eTaskGetState(counterTaskHandle) == eSuspended &&
          remainingCounterTime <= shortestTime) {
         shortestTime = remainingCounterTime;
         taskToResume = counterTaskHandle;
      }

      if (eTaskGetState(alphabetTaskHandle) == eSuspended &&
          remainingAlphabetTime <= shortestTime) {
         shortestTime = remainingAlphabetTime;
         taskToResume = alphabetTaskHandle;
      }

      // Resume selected task if one is available
      if (taskToResume != NULL) {
         vTaskResume(taskToResume);
      }

      // Small delay to allow selected task to execute
      vTaskDelay(50 / portTICK_PERIOD_MS);
   }
}

// ==================== Setup and Loop functions ====================
void setup() {

   Serial.begin(9600);

   Wire.begin(8, 9);
   lcd.init();
   delay(2);
   lcd.backlight();
   lcd.clear();

   pinMode(LED_PIN, OUTPUT);

   // Worker tasks (priority 1)
   xTaskCreatePinnedToCore(ledTask, "ledTask", 4096, NULL, 1, &ledTaskHandle, 0);
   xTaskCreatePinnedToCore(counterTask, "counterTask", 4096, NULL, 1, &counterTaskHandle, 0);
   xTaskCreatePinnedToCore(alphabetTask, "alphabetTask", 4096, NULL, 1, &alphabetTaskHandle, 0);

   // Scheduler task (higher priority to control execution order)
   xTaskCreatePinnedToCore(scheduleTasks, "scheduleTasks", 4096, NULL, 2, &schedulerTaskHandle, 0);
}


// Nothing in loop since all tasks are managed by FreeRTOS
void loop() {}