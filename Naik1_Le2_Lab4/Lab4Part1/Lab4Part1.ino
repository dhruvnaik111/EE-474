// Filename: Lab4Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/21/2026
// Description: Manage and schedule tasks with preemptive scheduling algorithms in freeRTOS
// ==================== Includes ====================
#include <stddef.h>

// ==================== Global Variables ====================

// Total times for tasks
const TickType_t ledTaskExecutionTime = 500 / portTICK_PERIOD_MS;      // 500 ms
const TickType_t counterTaskExecutionTime = 2000 / portTICK_PERIOD_MS; // 2 seconds
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS; // 13 seconds
// Remaining Execution Times
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

// Name: ledTask
// Description: Blink an LED and update remaining time for this task
void ledTask(void *arg) {

}

// Name: counterTask
// Description: Print out an incrementing counter to your LCD, and update remaining time for this task
void counterTask(void *arg) {

}

// Name: alphabetTask
// Description: Print out the alphabet to Serial, and update remaining time for this task
void alphabetTask(void *arg) {

}

// Name: scheduleTasks
// Description: Implement SRTF scheduling logic. This function should select the task with
//              the shortest remaining time and run it. Once a task completes it should
//              reset its remaining time.
void scheduleTasks(void *arg) {

}


void setup() {
   // TODO: Create 4 tasks and pin them to core 0:
   //          1. A scheduler that handles the scheduling of the other three tasks
   //          2. Blink an LED
   //          3. Print a counter to the LCD
   //          4. Print the alphabet to Serial
}
void loop() {}