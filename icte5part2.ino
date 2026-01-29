// Filename: icte5part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/29/2026
// Description: Implement a round-robin scheduler on the ESP32-S3 that will run multiple tasks using function pointers and structs.

// ==================== Includes ====================
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>

// ==================== Macros ====================
#define NTASKS 3
#define LED1_PIN 4
#define LED2_PIN 5

typedef struct TCBstruct {
    void (*ftpr)(void *p); // the function pointer
    void *arg_ptr; // the argument pointer
    unsigned short int state; // the task state
    unsigned int delay; // delay time in ms
} TCBstruct;

#define STATE_RUNNING 0
#define STATE_READY 1
#define STATE_WAITING 2
#define STATE_INACTIVE 3


// ==================== Global Variables ====================
TCBstruct TaskList[NTASKS];
unsigned int t_curr = 0; // current task index for round-robin

// ==================== Function Prototypes ====================
void taskA(void *p);
void taskB(void *p);
void executeTask(void (*taskFunction)(void *p), void *arg);
void scheduler();
void halt_me();
void start_task(int task_id);
void task_delay(int d);
// ==================== Functions ====================
void setup() {
    pinMode(LED1_PIN, OUTPUT); //LED1
    pinMode(LED2_PIN, OUTPUT); //LED2

    // Initialize task list
    int j = 0;
    
    // Task A: Blink LED1 at 500ms interval
    TaskList[j].ftpr = taskA;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;  // Pass task index
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    j++;
    
    // Task B: Blink LED2 at 300ms interval
    TaskList[j].ftpr = taskB;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;  // Pass task index
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    j++;
    
    // Third entry marks end of active tasks
    TaskList[j].ftpr = NULL;

}

void loop() {
    scheduler();
    delay(1); //each os tick is 1ms
}

// Name: executeTask
// Description: Executes a given task function passed as a function pointer
void executeTask(void (*taskFunction)(void *p), void *arg) {
    if(taskFunction != NULL) {
        taskFunction(arg);
    }
}

// Name: scheduler
// Description: Round-robin scheduler with delay counter for each task
void scheduler() {
    // Iterate through all tasks
    for(int i = 0; i < NTASKS - 1; i++) { // NTASKS - 1 because last entry is NULL
        if(TaskList[i].ftpr == NULL) break; // End of task list
        
        if(TaskList[i].state == STATE_READY) {
            // If delay is 0, execute the task
            if(TaskList[i].delay == 0) {
                TaskList[i].state = STATE_RUNNING;
                executeTask(TaskList[i].ftpr, TaskList[i].arg_ptr);
                
                // Set back to READY if still running
                if(TaskList[i].state == STATE_RUNNING) {
                    TaskList[i].state = STATE_READY;
                }
            } else {
                // Decrement delay counter
                TaskList[i].delay--;
            }
        }
        // Handle waiting tasks
        else if(TaskList[i].state == STATE_WAITING) {
            if(TaskList[i].delay == 0) {
                TaskList[i].state = STATE_READY;
            } else {
                TaskList[i].delay--;
            }
        }
    }
}

// Name: halt_me
// Description: Halts the currently running task
void halt_me() {
    TaskList[t_curr].state = STATE_INACTIVE;
}
// Name: start_task
// Description: Starts a task by setting its state to READY
void start_task(int task_id) {
    TaskList[task_id].state = STATE_READY;
}

// Name: task_delay
// Description: Delays the currently running task for d milliseconds
void task_delay(int d) {
    TaskList[t_curr].delay = d;
    TaskList[t_curr].state = STATE_WAITING;
}

// Name: taskA
// Description: Blinks LED1 (GPIO4) - resets its own delay counter
void taskA(void *p) {
    int task_id = (int)(intptr_t)p;
    digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
    TaskList[task_id].delay = 500;  // Reset delay for 500ms interval
}

// Name: taskB
// Description: Blinks LED2 (GPIO5) - resets its own delay counter
void taskB(void *p) {
    int task_id = (int)(intptr_t)p;
    digitalWrite(LED2_PIN, !digitalRead(LED2_PIN));
    TaskList[task_id].delay = 300;  // Reset delay for 300ms interval
}
