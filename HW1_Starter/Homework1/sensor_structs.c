// Filename: sensor_structs.c
// Author: Dhruv Naik
// Date: 01/19/2026
// Description: Working with structs and pointers to structs in C

// ====================includes====================
#include <stdio.h>
#include <string.h>
#include "hwlib.h"

// Step 1
// =================> TODO: Define a Sensor struct with int id, char label[10], float reading
// Name: Sensor
// Description: Struct representing a sensor with ID, label, and reading.
struct Sensor {
    int id;
    char label[10];
    float reading;
};

// Name: run_sensor_structs
// Description: Void inputs and outputs. Demonstrates struct usage and pointers to structs in C.
void run_sensor_structs() {
    // Step 1 Continued
    // =================> TODO: Declare a Sensor instance and set its fields
    struct Sensor s1;
    s1.id = 1;

    s1.label[0] = 'T';
    s1.label[1] = 'E';
    s1.label[2] = 'M';
    s1.label[3] = 'P';
    s1.label[4] = '\0';

    s1.reading = 25.6;

    // =================> TODO: Print its values
    printf("Sensor ID: %d, Label: %s, Reading: %.2f\n", s1.id, s1.label, s1.reading);

    // Step 2
    // =================> TODO: Declare a Sensor pointer pointing to the struct from Step 1
    struct Sensor* sptr = &s1;
    // =================> TODO: Use the pointer to modify reading and print again
    sptr->reading = 42.9;
    printf("Updated reading via pointer: %.2f\n", sptr->reading);
    printf("Sensor ID: %d, Label: %s, Reading: %.2f\n", sptr->id, sptr->label, sptr->reading);

    // Step 3
    // =================> TODO: Allocate memory for 3 Sensor structs
    printf("\n");
    printf("Allocating 3 sensors...\n");
    struct Sensor* sensors = (struct Sensor*) malloc(3 * sizeof(struct Sensor));
    // =================> TODO: Fill in ids, labels (T0, T1, T2), and readings

    for (int i = 0; i < 3; i++) {
        sensors[i].id = 100 + i;
        sensors[i].reading = 20.0f + 1.5f*i;

        snprintf(sensors[i].label,
                sizeof(sensors[i].label),
                "T%d", i);

    }

    // =================> TODO: Print all sensor info using pointer
    for (int i = 0; i < 3; i++) {
        printf("Sensor %d - ID: %d, Label: %s, Reading: %.2f\n",
            i,
            sensors[i].id,
            sensors[i].label,
            sensors[i].reading );
    }
    // =================> TODO: Free memory
    free(sensors);
}
