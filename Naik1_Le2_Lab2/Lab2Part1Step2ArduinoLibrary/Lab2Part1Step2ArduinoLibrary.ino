// Filename: Lab2Part1Step2ArduinoLibrary.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Compare the execution speed of arduino library functions vs direct register access

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
  // ================> TODO:
  // Set a pin as output
  pinMode(GPIO_PIN, OUTPUT); //LED1
  i = 0;
}
void loop() {
  // ================> TODO:
  // For 1000 repetitions:
  while(i < 1000) {
    //		Measure time to:
    startTime = micros();
    //        		- Turn pin's output to HIGH
    digitalWrite(GPIO_PIN, HIGH);
    endTime = micros();
    elapsedTime = endTime - startTime;
    totalTime += elapsedTime;
    //        		- Turn pin's output to LOW
    digitalWrite(GPIO_PIN, LOW);
    // 1 second delay
    delay(1000);
    i++;
  }
  
  // Print out total time to the serial monitor
   Serial.println(totalTime);
}