#include <stdio.h>
#include "hwlib.h"

// TODO: Implement int sum_array(const int* arr, int size)
// TODO: Implement int find_max(const int* arr, int size)
// TODO: Implement int filter_positive(const int* in, int* out, int size)
// TODO: Implement int find_max_offset(const int* arr, int size, int offset)

void run_array_ops() {
    // TODO: declare array
    int arr[] = {1, -2, 3, -4, 5, -6};
    int size = 6;
     // TODO: Print out contents of original array
    printf("Original array: %d %d %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
   
    printf("\n");

    // TODO: Call and print results from sum_array, find_max, filter_positive, and find_max_offset
    int sum = sum_array(arr, size);
    printf("Sum = %d\n", sum);

    int max = find_max(arr, size);
    printf("Max = %d\n", max);

    int filtered[6];
    int count = filter_positive(arr, filtered, size);
    printf("Filtered positives: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", filtered[i]);
    }
    printf("\n");
}

int sum_array(const int* arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int find_max(const int* arr, int size) {
     int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

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

int find_max_offset(const int* arr, int size, int offset) {
    int max = *arr;
    for (const int* ptr = arr + offset; ptr < arr + size; ptr += offset) {
        if (*ptr > max) {
            max = *ptr;
        }
    }
    return max;
}
