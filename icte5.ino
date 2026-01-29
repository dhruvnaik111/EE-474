// Filename: icte5.ino
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
bool time_to_sleep = false;
void (*waitingtasks[NTASKS])(void *p);
// ==================== Function Prototypes ====================
void taskA(void *p);
void taskB(void *p);
void executeTask(void (*taskFunction)());
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
    delay(1000); // Delay to simulate time between scheduler runs
}

// Name: executeTask
// Description: Executes a given task function passed as a function pointer
void executeTask(void (*taskFunction)()) {
  taskFunction();
}

void scheduler() {
    if(readytasks[task_index] == NULL && task_index != 0) {
        task_index=0;
    }
    if(readytasks[task_index] == NULL && task_index == 0){
    // figure out something to do because
    // there are no tasks to run!
    } else {
        executeTask(readytasks[task_index]);
        task_index++;
    }
    // Round Robin/we’re taking turns
    return;
}

void halt_me() {
    // 1. Identify which task is currently running (i.e.
    // look at task_index)
    // 2. Copy the function pointer from
    // readytasks[task_index] to the
    // haltedtasks array
    // 3. Move the remaining tasks up in readytasks[] to
    // fill the empty hole and copy NULL into the
    // last element.
    waitingtasks[task_index] = readytasks[task_index];
    for(int i = task_index; i < NTASKS - 1; i++) {
        readytasks[i] = readytasks[i+1];
    }
    readytasks[NTASKS - 1] = NULL;
    return;
}

// Name: taskA
void taskA(void *p) {
    //do computation
    if(time_to_sleep){
        sleep(10);
        return;
    }
    else {
        // compute for a little while
    }
    return;
}

// Name: taskB
void taskB(void *p) {
    //do computation
    if(time_to_sleep){
        sleep(5);
        return;
    }
    else {
        // compute for a little while
    }
    return;
}
void sleep(int d) {
    // 1. Copy function pointer from
    // readytasks[task_index] to the
    // waitingtasks[] array.
    // 2. Clean up readytasks[] as in halt_me();
    // copy d into the delays array with the same
    // index as the function pointer has in
    // waitingtasks[]
    // 5. for each element of waitingtasks which is
    // not NULL: decrement the delay value d.
    // if (d==0) move the function pointer to the
    // end of the readytasks[] array and remove
    // it from waitingtasks.
    // end of scheduler
    waitingtasks[task_index] = readytasks[task_index];
    for(int i = task_index; i < NTASKS - 1; i++) {
        readytasks[i] = readytasks[i+1];
    }
    readytasks[NTASKS - 1] = NULL;

    return;
}