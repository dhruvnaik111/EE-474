// Filename: intro_pointers.c
// Author: Dhruv Naik
// Date: 01/19/2026
// Description: Introduction to pointers in C


// ====================includes====================
#include <stdio.h>
#include <string.h>
#include "hwlib.h"

// Name: run_intro_pointers
// Description: Void inputs and outputs. Demonstrates basic pointer usage in C.
void run_intro_pointers() {
    // Step 1
    // =================> TODO: Declare an int a and set it to 42
    int a = 42;
    // =================> TODO: Create a pointer p to a
    int *p = &a;
    // =================> TODO: Use p to reassign a to the value of 100
    *p = 100;
    // =================> TODO: Print a and *p, and the address of a
    printf("a = %d, *p = %d\n", a, *p);
    printf("Address of a: %p\n", (void*)p);
    // Step 2
    // =================> TODO: Create another pointer q and set it equal to p
    int *q = p;
    // =================> TODO: Use q to change value of a
    *q = 77;
    // =================> TODO: Print a, *p, and *q
    printf("a = %d, *p = %d, *q = %d\n", a, *p, *q);
    // Step 3
    // =================> TODO: Declare b = 200
    int b = 200;
    printf("Original b = %d\n", b);
    // =================> TODO: Reassign p to point at b
    p = &b;
    // =================> TODO: Use p to reassign b to be the value 999
    *p = 999;
    // =================> TODO: Print a and b
    printf("New b = %d, and a = %d\n", b, a);
}
