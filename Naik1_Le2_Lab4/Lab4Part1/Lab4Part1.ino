/**
 * @file Lab4Part1.ino
 * @authors Dhruv Naik, Ethan Le
 * @date 02/28/2026
 * @brief Manage and schedule tasks with preemptive scheduling algorithms in FreeRTOS
 * @details Implements Shortest Remaining Time First (SRTF) scheduling to manage
 * three concurrent tasks: LED control, counter display, and alphabet printing.
 */ 


// ==================== Includes ====================
#include <stddef.h>
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================
#define LED_PIN 2

// ==================== Global Constants ====================
const TickType_t ledTaskExecutionTime = 500 / portTICK_PERIOD_MS;
const TickType_t counterTaskExecutionTime = 2000 / portTICK_PERIOD_MS;
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS;

// ==================== Global Variables ====================
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

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

void loop() {}

/**
 * @brief Toggle an LED when its execution period expires
 * @details Executes for 500ms every scheduling cycle. The task maintains its own
 * remaining execution time, which decreases by 250ms per slice, and suspends itself
 * when the slice completes to allow the scheduler to select the next task.
 * @param arg Unused task parameter (pointer to void)
 */
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

/**
 * @brief Display an incrementing counter on the LCD every 2 seconds
 * @details Executes every 2 seconds to display a counter value (1-20) on the LCD.
 * Updates the remaining execution time before suspending. Counter automatically
 * resets to 0 after reaching 20.
 * @param arg Unused task parameter (pointer to void)
 */
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

/**
 * @brief Print letters A through Z to Serial every 13 seconds
 * @details Executes every 13 seconds to print the next letter in the alphabet sequence.
 * After reaching 'Z', the sequence automatically restarts at 'A'. Updates remaining
 * execution time before suspending.
 * @param arg Unused task parameter (pointer to void)
 */
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

/**
 * @brief Implement Shortest Remaining Time First (SRTF) scheduling algorithm
 * @details Scheduler examines all suspended tasks and resumes the task with the
 * smallest remaining execution time. This ensures fair allocation and preemptive
 * scheduling. Task remaining times are reset within each task when execution completes.
 * @param arg Unused task parameter (pointer to void)
 */
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
