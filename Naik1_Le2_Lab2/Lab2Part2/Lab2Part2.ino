// Filename: Lab2Part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Directly interact with the ESP32's timer registers to control an LED

// ==================== Includes ====================
#include "soc/timer_group_reg.h"

// ==================== Macros ====================
#define GPIO_PIN 5

void setup() {
  Serial.begin(9500);

  pinMode(GPIO_PIN, OUTPUT);
    
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) &= ~(0xFFFF << 13); //Clear divider bits
    
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (80 << 13); // Set clock divider to 1MHz

  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1 << 30); //Counter increases

  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1U << 31); //Start the timer

  *((volatile uint32_t *)TIMG_T0LOADLO_REG(0)) &= 0x00000000; //Reset the timer to this value (0)

  digitalWrite(GPIO_PIN, HIGH); //Start LED as on
}

void loop() {

  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1; //Trigger Update to read timer value

  uint32_t currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  //Serial.println(currentTime);

    if (currentTime >= 500000) { //If its been 0.5 seonds
        digitalWrite(GPIO_PIN, !digitalRead(GPIO_PIN)); //toggle LED
        *((volatile uint32_t *)TIMG_T0LOAD_REG(0)) = 1; //Reset the timer
        //Serial.println("Restart");
    }
}
