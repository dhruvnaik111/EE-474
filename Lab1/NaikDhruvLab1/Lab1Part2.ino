// Filename: Lab1Part2.ino
// Author: Dhruv Naik
// Date: 01/12/2026
// Description: This file controls an LEDs on an ESP32 when pressed by a pushbutton

// ============================= Setup =======================================
void setup() {
  pinMode(19, OUTPUT); //Set pin 19 to output (LED)
  pinMode(4, INPUT_PULLUP); //Set pin 4 to input (Button)
}

// ============================= Loop =======================================
void loop() {
  if (digitalRead(4) == LOW) { //if button is pressed
  digitalWrite(19, HIGH); //turn on LED
  } else { //otherwise
  digitalWrite(19, LOW); //turn off LED
  }

}
