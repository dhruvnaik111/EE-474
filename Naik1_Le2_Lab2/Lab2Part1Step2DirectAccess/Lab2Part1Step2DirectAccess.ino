// Filename: Lab2Part1Step2DirectAccess.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Compare the execution speed of arduino library functions

// ==================== Includes ====================
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"

// ==================== Macros ====================
#define GPIO_PIN 5 // GPIO5

// ==================== Global Variables ====================
unsigned long startTime, endTime, elapsedTime, totalTime;
int i;

// ==================== Functions ====================
void setup() {
  Serial.begin(9600);
  delay(5000);
  // Set a pin as GPIO
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);
  // Set a pin as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);
  i = 0;
}
void loop() {
  // For 1000 repetitions:
  while(i < 1000) {
    //		Measure time to:
    startTime = micros();
    //  - Turn pin's output to HIGH
    *((volatile uint32_t *)GPIO_OUT_REG) |= (1 << GPIO_PIN);
    endTime = micros();
    elapsedTime = endTime - startTime;
    totalTime += elapsedTime;
    //   - Turn pin's output to LOW
    *((volatile uint32_t *)GPIO_OUT_REG) &= ~(1 << GPIO_PIN);
    // 1 second delay
    delay(1000);
    i++;
  }
  
  // Print out total time to the serial monitor
   Serial.println(totalTime);
}
