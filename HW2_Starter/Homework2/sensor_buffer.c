// Filename: sensor_buffer.c
// Author: Dhruv Naik
// Date: 02/14/2026
// Description: Implements a circular buffer for storing sensor data, including functions to initialize the buffer, check if it's empty or full, push new values, pop values, and print the buffer contents.

// ==================== Includes ====================
#include <stdio.h>
#include <stdbool.h>
#include "hwlib.h"

typedef struct {
    float buffer[5];
    int head;
    int tail;
} SensorBuffer;

// ==================== Function prototypes ====================
void init_buffer(SensorBuffer* buf);
bool is_empty(SensorBuffer* buf);
bool is_full(SensorBuffer* buf);
bool push(SensorBuffer* buf, float value);
bool pop(SensorBuffer* buf, float* out);
void print_buffer(SensorBuffer* buf);

// ==================== Functions ====================

// Name: run_sensor_buffer
// Description: Demonstrates the circular buffer behavior for sensor data
void run_sensor_buffer() {
    // TODO: Demonstrate the circular buffer behavior described in the assignment
    SensorBuffer buf;
    init_buffer(&buf);
    //push 5 values
    push(&buf, 1.0);
    push(&buf, 2.0);
    push(&buf, 3.0);
    push(&buf, 4.0);
    push(&buf, 5.0);

    //print buffer
    print_buffer(&buf);

    //pop 2 values
    float val;
    pop(&buf, &val);
    pop(&buf, &val);
    //push 2 new values
    push(&buf, 5.0);
    push(&buf, 6.0);

    //print buffer
    print_buffer(&buf);
}

// Name: init_buffer
// Description: Initializes the sensor buffer by setting head and tail indices to 0
void init_buffer(SensorBuffer* buf) {
    buf->head = 0;
    buf->tail = 0;
}

// Name: is_empty
// Description: Checks if the buffer is empty by comparing head and tail indices
bool is_empty(SensorBuffer* buf) {
    return buf->head == buf->tail;
}

// Name: is_full
// Description: Checks if the buffer is full by comparing head and tail indices with wrap-around
bool is_full(SensorBuffer* buf) {
    return (buf->head + 1) % 5 == buf->tail;
}

// Name: push
// Description: Adds a new value to the buffer if it's not full, and updates the head index
bool push(SensorBuffer* buf, float value) {
    if (is_full(buf)) {
        printf("Buffer full, could not push %.2f\n", value);
        return false; // Buffer is full
    }
    buf->buffer[buf->head] = value;
    buf->head = (buf->head + 1) % 5;
    printf("Pushed %.1f\n", value);
    return true;
}

// Name: pop
// Description: Removes a value from the buffer if it's not empty, stores it in the provided output variable, and updates the tail index
bool pop(SensorBuffer* buf, float* out) {
    if (is_empty(buf)) {
        return false; // Buffer is empty
    }
    *out = buf->buffer[buf->tail];
    buf->tail = (buf->tail + 1) % 5;
    printf("Popped %.1f\n", *out);
    return true;
}

// Name: print_buffer
// Description: Prints the contents of the buffer from tail to head, accounting for wrap-around
void print_buffer(SensorBuffer* buf) {
    printf("Buffer contents: ");
    int index = buf->tail;
    while (index != buf->head) {
        printf("%.1f ", buf->buffer[index]);
        index = (index + 1) % 5;
    }
    printf("\n");
}

