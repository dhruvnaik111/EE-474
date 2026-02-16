#include <stdio.h>
#include <stdbool.h>
#include "hwlib.h"

typedef struct {
    float buffer[5];
    int head;
    int tail;
} SensorBuffer;

// TODO: Implement void init_buffer(SensorBuffer* buf)
// TODO: Implement bool is_empty(SensorBuffer* buf)
// TODO: Implement bool is_full(SensorBuffer* buf)
// TODO: Implement bool push(SensorBuffer* buf, float value)
// TODO: Implement bool pop(SensorBuffer* buf, float* out)
// TODO: Implement void print_buffer(SensorBuffer* buf)

void run_sensor_buffer() {
    // TODO: Demonstrate the circular buffer behavior described in the assignment
}
