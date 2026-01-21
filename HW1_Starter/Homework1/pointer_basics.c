// Filename: pointer_basics.c
// Author: Dhruv Naik
// Date: 01/19/2026
// Description: Basics of pointers and memory manipulation in C

// ====================includes====================
#include <stdio.h>
#include <string.h>
#include "hwlib.h"

// Name: my_memcpy
// Description: Takes in destination pointer, source pointer, and size in bytes to copy memory from source to destination.
void my_memcpy(void* dest, const void* src, size_t size) {
    // =================> TODO: Implement byte-by-byte memory copy using uint8_t*
    uint8_t *dest_mem = (uint8_t*)dest;
    const uint8_t *src_mem = (uint8_t*)src;

    for (size_t i = 0; i < size; i++) {
        dest_mem[i] = src_mem[i];
    }
}

// Name: run_pointer_basics
// Description: Void inputs and outputs. Demonstrates pointer basics and memory copy in C.
void run_pointer_basics() {

    // Step 1
    // =================> TODO: Declare and initialize an int[5] array
    int arr_int[5] = {10, 20, 30, 40, 50};
    // =================> TODO: Declare and initialize a char[5] array
    char arr_char[5] = {'a', 'b', 'c', 'd', 'e'};
    
    // =================> TODO: Print value and address of each element using a for loop
    printf("Integer array:\n");
    for (int i = 0; i < 5; i++) {
        printf("arr[%d] = %d, address: %p\n", i, arr_int[i], (void*)&arr_int[i]);
    }
    printf("\n");
    printf("Char array:\n");
    for (int i = 0; i < 5; i++) {
        printf("text[%d] = %c, address: %p\n", i, arr_char[i], (void*)&arr_char[i]);
    }
    // Step 2
    // =================> TODO: Declare source string and destination buffer
    char str[] = "hello world";
    char dest[20];
    // =================> TODO: Use my_memcpy() to copy string
    my_memcpy(dest, str, strlen(str) + 1);
    // =================> TODO: Print copied string
    printf("\n");
    printf("Copied string: %s\n", dest);
}
