// Filename: Lab3Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/07/2026
// Description: 

// ==================== Includes ====================
#include <Wire.h>
// Ensure that the LiquidCrystal I2C library by Frank de Brabander is installed in your Arduino IDE Library Manager
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================


// ==================== Functions ====================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

void setup() {
 Serial.begin(115200);
 Wire.begin();
 lcd.init();
 delay(2);
}


void loop() {
 // ====================> TODO:
 // Write code that takes Serial input and displays it to the LCD. Do NOT use any 
 // functions from the LiquidCrystal I2C library here.

 
}
