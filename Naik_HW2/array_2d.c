// Filename: array_2d.c
// Author: Dhruv Naik
// Date: 02/14/2026
// Description: Implements operations on 2D arrays, including printing elements using different methods and zeroing out the array using a helper function.

// ==================== Includes ====================
#include <stdio.h>
#include "hwlib.h"

// ==================== Function prototypes ====================
void zero_2d(int* data, int rows, int cols);

// ==================== Functions ====================
// Name: run_array_2d
// Description: Demonstrates different methods of accessing and printing a 2D array
void run_array_2d() {
    // TODO: Declare and print matrix[2][3] using different methods
    int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};

    //array indexing
    printf("Matrix using array indexing:\n");
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            printf("matrix[%d][%d] = %d at %p\n", i, j, matrix[i][j], &matrix[i][j]);
        }
    }

    //pointer arithmetic
    printf("\nMatrix using flattened pointer arithmetic: \n");
    int* ptr = &matrix[0][0];
    for (int k = 0; k < 6; k++) {
        int i = k / 3;
        int j = k % 3;
        printf("matrix[%d][%d] = %d at %p\n", i, j, *(ptr + k), (void *)(ptr +k));
    }

    // TODO: Call zero_2d and reprint
    zero_2d(&matrix[0][0], 2, 3);
    printf("\nZeroed matrix:\n");

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Name: zero_2d
// Description: Sets all elements of a 2D array passed as a flattened pointer to zero
void zero_2d(int* data, int rows, int cols) {
    for (int *p = data; p < data + rows * cols; p++) {
        *p = 0;
    }
}

