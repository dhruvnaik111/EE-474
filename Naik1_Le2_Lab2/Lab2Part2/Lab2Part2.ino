// Filename: Lab2Part2.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Directly interact with the ESP32's timer registers to control an LED

// ==================== Includes ====================
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"

// ==================== Macros ====================
#define GPIO_PIN 5
bool state;
bool fired = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Start Lab2Part2.ino");
  delay(1000);
  // Set a pin as GPIO
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);
  // Set a pin as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);
    
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) &= ~(0xFFFF << 13); //Clear divider bits
    
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (80 << 13); // Set clock divider to 1MHz

  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1 << 30); //Counter increases

  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1U << 31); //Start the timer

  state = true;
}

void loop() {

  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1; //Trigger Update to read timer value

  uint32_t currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  //Serial.println(currentTime);

    if (currentTime >= 1000000 && !fired) { //If its been 1 second
        fired = true;

        if (state){ //toggle led based on current state
          *((volatile uint32_t *)GPIO_OUT_REG) |= (1 << GPIO_PIN);
          state = false;
        } else {
          *((volatile uint32_t *)GPIO_OUT_REG) &= ~(1 << GPIO_PIN);
          state = true;
        }

        *((volatile uint32_t *)TIMG_T0LOADLO_REG(0)) &= 0x00000000;
        *((volatile uint32_t *)TIMG_T0LOADHI_REG(0)) &= 0x00000000;
        *((volatile uint32_t *)TIMG_T0LOAD_REG(0)) |= 0xFFFFFFFF; // load 0 into the timer register

        //Serial.println(state);
        //Serial.println("Restart timer");
    }

    if (currentTime < 1000) {
      fired = false;  // reset flag
    }
    
}
