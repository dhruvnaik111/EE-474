#ifndef HWLIB_H
#define HWLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <string.h>

// printf wrapper function for C files
void hw_printf(const char* format, ...);

// define printf as hw_printf in C files
#define printf hw_printf

// delay wrapper
#define delay_ms(ms) delay(ms)


#ifdef __cplusplus
}
#endif

#endif

