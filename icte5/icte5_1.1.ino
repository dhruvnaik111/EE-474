// Filename: icte5_1.1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/29/2026
// Description: Implement a round-robin scheduler on the ESP32-S3 that will run multiple tasks using function pointers.

// ==================== Includes ====================
#include <stddef.h>
#include <Arduino.h>

// ==================== Macros ====================
#define NTASKS 3
#define LED1_PIN 2
#define LED2_PIN 4

// ==================== Global Variables ====================
void (*readytasks[NTASKS])(void *p);
int task_index = 0;

// ==================== Function Prototypes ====================
void taskA(void *p);
void taskB(void *p);
void executeTask(void (*taskFunction)(void *p));
void scheduler();

// ==================== Functions ====================
void setup() {
    readytasks[0] = taskA;
    readytasks[1] = taskB;
    readytasks[2] = NULL; // No third task
    pinMode(LED1_PIN, OUTPUT); //LED1
    pinMode(LED2_PIN, OUTPUT); //LED2
}

void loop() {
    scheduler();
}

// Name: executeTask
// Description: Executes a given task function passed as a function pointer
void executeTask(void (*taskFunction)(void *p)) {
    if (taskFunction != NULL) {
        taskFunction(nullptr);
    }
}

// Name: scheduler
// Description: Simple round-robin scheduler
void scheduler() {
    if(readytasks[task_index] == NULL && task_index != 0) {
        task_index=0;
    }
    if(readytasks[task_index] == NULL && task_index == 0) {
        // figure out something to do because
        // there are no tasks to run!
    } else {
        executeTask(readytasks[task_index]);
        task_index++;
    }
    // Round Robin/we’re taking turns
    return;
}

// Name: taskA
// Description: Toggles LED1 every 500ms
void taskA(void *p) {
    (void)p;
    static unsigned long lastToggleMs = 0;
    static bool ledState = false;
    const unsigned long intervalMs = 500;

    unsigned long now = millis();
    if (now - lastToggleMs >= intervalMs) {
        lastToggleMs = now;
        ledState = !ledState;
        digitalWrite(LED1_PIN, ledState ? HIGH : LOW);
    }
}

// Name: taskB
// Description: Toggles LED2 every 200ms
void taskB(void *p) {
    (void)p;
    static unsigned long lastToggleMs = 0;
    static bool ledState = false;
    const unsigned long intervalMs = 200; // different from taskA

    unsigned long now = millis();
    if (now - lastToggleMs >= intervalMs) {
        lastToggleMs = now;
        ledState = !ledState;
        digitalWrite(LED2_PIN, ledState ? HIGH : LOW);
    }
}