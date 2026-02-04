// Filename: Lab3Part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/07/2026
// Description: Manage multiple tasks using a basic non-preemptive scheduling algorithm utalizing Task Control Blocks.

// ==================== Includes ====================
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================
#define NTASKS 5
#define LED1_PIN 4
#define BUZZER_PIN 6

typedef struct TCBstruct {
    void (*ftpr)(void *p); // the function pointer
    void *arg_ptr; // the argument pointer
    unsigned short int state; // the task state
    unsigned int delay; // delay time in ms
    int priority; // task priority
    bool completed;
    const char *name; // task name
} TCBstruct;

#define STATE_RUNNING 0
#define STATE_READY 1
#define STATE_WAITING 2
#define STATE_INACTIVE 3


// ==================== Global Variables ====================
TCBstruct TaskList[NTASKS];
unsigned int t_curr = 0; // current task index

// ==================== Function Prototypes ====================
void LEDBlinkerTask(void *p);
void CounterTask(void *p);
void MusicPlayerTask(void *p);
void AlphabetPrinterTask(void *p);

void executeTask(void (*taskFunction)(void *p), void *arg);
void scheduler();
void halt_me();
void start_task(int task_id);
void task_delay(int d);

// ==================== Functions ====================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

void setup() {
    // Initialize LED
    pinMode(LED1_PIN, OUTPUT);

    // Initialize Serial
    Serial.begin(9600);

    // Initialize LCD
    Wire.begin(8,9);
    lcd.init();
    delay(2);
    lcd.backlight();
    lcd.clear();

    // Initialize PWM for Buzzer
    ledcAttach(BUZZER_PIN, 1000, 8); //sets pin as PWM

    Serial.println("Start of Lab3Part2");

    // Initialize task list
    int j = 0;
    
    // Led Blinker Task
    TaskList[j].ftpr = LEDBlinkerTask;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    TaskList[j].priority = 1;
    TaskList[j].completed = false;
    TaskList[j].name = "LED Blinker";
    j++;
    
    // Counter Task
    TaskList[j].ftpr = CounterTask;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;  // Pass task index
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    TaskList[j].priority = 2;
    TaskList[j].completed = false;
    TaskList[j].name = "Counter";
    j++;

    // Music Player Task
    TaskList[j].ftpr = MusicPlayerTask;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;  // Pass task index
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    TaskList[j].priority = 3;
    TaskList[j].completed = false;
    TaskList[j].name = "Music Player";
    j++;
    
    // Alphabet Printer Task
    TaskList[j].ftpr = AlphabetPrinterTask;
    TaskList[j].arg_ptr = (void*)(intptr_t)j;  // Pass task index
    TaskList[j].state = STATE_READY;
    TaskList[j].delay = 0;
    TaskList[j].priority = 4;
    TaskList[j].completed = false;
    TaskList[j].name = "Alphabet Printer";
    j++;
    

    // Fifth entry marks end of active tasks
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
// Description: Priority based non-preemptive scheduler using Task Control Blocks
void scheduler() {
    int selected = -1;
    bool taskInProgress = false;

    // Update waiting timers for all tasks
    for (int i = 0; i < NTASKS - 1; i++) {
        if (TaskList[i].ftpr != NULL && TaskList[i].state == STATE_WAITING) {
            if (TaskList[i].delay > 0) TaskList[i].delay--;
            if (TaskList[i].delay == 0) TaskList[i].state = STATE_READY;
        }
    }

    // Loop through priority levels 1 through 4
    for (int p = 1; p <= 4; p++) {
      // Check each task for the current priority level
        for (int i = 0; i < NTASKS - 1; i++) {
            // Find the task matching the current priority level we are checking
            if (TaskList[i].ftpr != NULL && TaskList[i].priority == p && !TaskList[i].completed) {
                
                taskInProgress = true; // An incomplete task exists at this priority

                if (TaskList[i].state == STATE_READY) {
                    selected = i; // This is the highest priority task ready to run
                } 
                
                // Break out of both loops to execute the task
                p = 5; // Breaks the outer loop
                break; // Breaks the inner loop
            }
        }
    }

    // Task Execution
    if (selected != -1) {
        t_curr = selected;
        TaskList[selected].state = STATE_RUNNING;
        executeTask(TaskList[selected].ftpr, TaskList[selected].arg_ptr);

        // Return to READY if the task didn't halt itself
        if (TaskList[selected].state == STATE_RUNNING) {
            TaskList[selected].state = STATE_READY;
        }

        // Handle task completion
        if (TaskList[selected].state == STATE_INACTIVE) {
            TaskList[selected].completed = true;
            Serial.print(TaskList[selected].name);
            Serial.print(": ");
            Serial.println(TaskList[selected].priority);
        }
    }

    // Change priorities once all tasks finish
    if (!taskInProgress) {
        //Serial.println("Priority Rotated");
        for (int i = 0; i < NTASKS - 1; i++) {
            if (TaskList[i].ftpr == NULL) break;
            TaskList[i].completed = false;
            TaskList[i].state = STATE_READY;
            TaskList[i].delay = 0;
            TaskList[i].priority = (TaskList[i].priority % 4) + 1;
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

// Name: LEDBlinkerTask
// Description: Blinks LED1 (GPIO4) - resets its own delay counter
void LEDBlinkerTask(void *p) {
    int task_id = (int)(intptr_t)p;
    static int togglecount = 0;

    digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
    togglecount++;

    if (togglecount >= 16) {
      togglecount = 0;
      halt_me(); // Stop after 8 on off cycles
      return;
    }

    task_delay(1000);  // Reset delay for 1s interval
}

// Name: CounterTask
// Description: Count up from 1 to 10 on your LCD.
void CounterTask(void *p) {
    static int count = 1;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Count: ");
    lcd.print(count);

    count++;

    if (count > 10) {
        count = 1;
        halt_me(); // Task completed
        return;
    }

    task_delay(500);
}

// Name: MusicPlayerTask
// Description: Play a melody of ten notes on a buzzer and display the “notes” (read: different voltage levels) on a second LED.
void MusicPlayerTask(void *p) {
    static int note_index = 0;

    // Ten "notes" represented as duty cycles (voltage levels)
    uint8_t note_levels[10] = {
        25, 50, 75, 100, 125,
        150, 175, 200, 225, 255
    };

    // Output voltage level to LED
    ledcWrite(BUZZER_PIN, note_levels[note_index]);

    note_index++;

    if (note_index >= 10) {
        ledcWrite(BUZZER_PIN, 0); // Turn LED off
        note_index = 0;
        halt_me(); // Task complete
        return;
    }

    task_delay(300); // Time between "notes"
}

// Name: AlphabetPrinterTask
// Description: Print A, B, C…, Z to your serial monitor.
void AlphabetPrinterTask(void *p) {
    static char letter = 'A';

    Serial.print(letter);
    Serial.print(", ");

    if (letter == 'Z') {
        Serial.println();
        letter = 'A';
        halt_me(); // Task completed
        return;
    }

    letter++;
    task_delay(200);
}
