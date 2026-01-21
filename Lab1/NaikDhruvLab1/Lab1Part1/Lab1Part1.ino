// Filename: Lab1Part1.ino
// Author: Dhruv Naik
// Date: 01/12/2026
// Description: This file controls the blinking of 2 LEDs on an ESP32 at different frequencies

// ============================= Global Variables =======================================
// frequencies of blinking
unsigned long interval1 = 2000;  // LED1: 2 seconds
unsigned long interval2 = 500;   // LED2: 0.5 seconds

//time since the previous state
unsigned long prev1 = 0;
unsigned long prev2 = 0;

//the LEDs current state 
bool state1 = LOW;
bool state2 = LOW;

// ============================= Setup =======================================
void setup() {
  pinMode(19, OUTPUT); //LED1
  pinMode(21, OUTPUT); //LED2
}

// ============================= Loop =======================================
void loop() {
  unsigned long now = millis();

  //LED1
  if (now - prev1 >= interval1) { //(now - prev) is the time since the last change
    prev1 = now;
    state1 = !state1; //change LED state
    digitalWrite(19, state1);
  }

  //LED2 (same logic as LED1)
  if (now - prev2 >= interval2) {
    prev2 = now;
    state2 = !state2;
    digitalWrite(21, state2);
  }
}