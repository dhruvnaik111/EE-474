// Filename: array_ops.c
// Author: Dhruv Naik
// Date: 02/14/2026
// Description: Implements various operations on arrays, including summing elements, finding the maximum value, filtering positive values, and finding the maximum value with a specified offset.

// ==================== Includes ====================
#include <stdio.h>
#include "hwlib.h"


// ==================== Function Prototypes ====================
int sum_array(const int* arr, int size);
int find_max(const int* arr, int size);
int filter_positive(const int* in, int* out, int size);
int find_max_offset(const int* arr, int size, int offset);

// ==================== Functions ====================

// Name: run_array_ops
// Description: Runs all array operations and prints results
void run_array_ops() {
    // TODO: declare array
    int arr[] = {1, -2, 3, -4, 5, -6};
    int size = 6;
     // TODO: Print out contents of original array
    printf("Original array: %d %d %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);

    // TODO: Call and print results from sum_array, find_max, filter_positive, and find_max_offset
    int sum = sum_array(arr, size);
    printf("Sum = %d\n", sum);

    int max = find_max(arr, size);
    printf("Max = %d\n", max);

    int filtered[6];
    int count = filter_positive(arr, filtered, size);
    printf("Filtered positives (%d): ", count);
    for (int i = 0; i < count; i++) {
        printf("%d ", filtered[i]);
    }
    printf("\n");

    int max_offset = find_max_offset(arr, size, 2);
    printf("Max with offset 2: %d\n", max_offset);
}

// Name: sum_array
// Description: Returns the sum of all elements in the array
int sum_array(const int* arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

// Name: find_max
// Description: Returns the maximum value in the array
int find_max(const int* arr, int size) {
     int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

// Name: filter_positive
// Description: Copies positive values from the input array to the output array and returns the count of positive values
int filter_positive(const int* in, int* out, int size) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (in[i] > 0) {
            out[count] = in[i];
            count++;
        }
    }
    return count;
}

// Name: find_max_offset
// Description: Returns the maximum value in the array considering only elements at specified offsets
int find_max_offset(const int* arr, int size, int offset) {
    int max = *arr;
    for (const int* ptr = arr + offset; ptr < arr + size; ptr += offset) {
        if (*ptr > max) {
            max = *ptr;
        }
    }
    return max;
}
