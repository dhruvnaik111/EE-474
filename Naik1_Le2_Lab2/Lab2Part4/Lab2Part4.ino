// Filename: Lab2Part4.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Use a photoresistor to control the brightness of an LED

// ==================== Includes ====================
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"

// ==================== Macros ====================
#define BUZZER_PIN 5
#define PHOTORESISTOR_PIN 1
bool started;
// ==================== Functions ====================
void setup(){
  Serial.begin(9600);
  Serial.println("Start Part 4");
  ledcAttach(BUZZER_PIN, 1000, 8); //sets pin as PWM

  //Timer
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) &= ~(0xFFFF << 13); //Clear divider bits  
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (80 << 13); // Set clock divider to 1MHz
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1 << 30); //Counter increases
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) |= (1U << 31); //Start the timer
  started = false;
}

void loop() {
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1; //Trigger Update to read timer value
  uint32_t currentTime = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  int lightLevel = analogRead(PHOTORESISTOR_PIN);
  bool play = false;
  if (lightLevel <= 2047) {
    play = true;
    //Serial.println(lightLevel);
  } else {
    //Serial.println(lightLevel);
  }
  if (play) {
    if(!started) {
      *((volatile uint32_t *)TIMG_T0LOADLO_REG(0)) &= 0x00000000;
      *((volatile uint32_t *)TIMG_T0LOADHI_REG(0)) &= 0x00000000;
      *((volatile uint32_t *)TIMG_T0LOAD_REG(0)) |= 0xFFFFFFFF; // load 0 into the timer register to start the timer at 0
      started = true;
      Serial.println("started");
    }
    if(currentTime < 1000000){
      ledcWrite(BUZZER_PIN, 64);
      Serial.println("first stage");
    } else if (currentTime < 2000000){
      ledcWrite(BUZZER_PIN, 128);
      Serial.println("second stage");
    } else if(currentTime < 3000000){
      ledcWrite(BUZZER_PIN, 192);
      Serial.println("third stage");
    } else {
      ledcWrite(BUZZER_PIN, 0);
    }
  } else {
    ledcWrite(BUZZER_PIN, 0);
    started = false;
  }
  //Serial.println(currentTime);
  //int duty = map(lightLevel, 0, 4095, 0, 255); //map values to 0-255
  //ledcWrite(LED_PIN, duty); // write output
}