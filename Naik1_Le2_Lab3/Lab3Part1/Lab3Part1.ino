// Filename: Lab3Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/07/2026
// Description: 

// ==================== Includes ====================
#include <Wire.h>
// Ensure that the LiquidCrystal I2C library by Frank de Brabander is installed in your Arduino IDE Library Manager
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================
#define RegSel 0x01
#define ReadWrite 0x02
#define Enable 0x04
#define Backlight 0x08

#define LCD_ADDR 0x27

// ==================== Global Variables ====================
String text = "";

// ==================== Functions ====================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

void setup() {
    Serial.begin(115200);
    Wire.begin(8,9);
    lcd.init();
    delay(2);

    Serial.println("Type a message to display: ");
}


void loop() {
 // ====================> TODO:
 // Write code that takes Serial input and displays it to the LCD. Do NOT use any 
 // functions from the LiquidCrystal I2C library here.
    String displaytext = "";

    if(Serial.available() > 0) {
        char c = Serial.read();
        if (c != '\n') {
            text += c;
        } else {
            // Clear LCD Display
            displaytext = text;
            text = "";
            Serial.println("Displaying on LCD: " + displaytext);

            // Send each character to the LCD

        }
   
    }
 
}

// Name: lcdWrite
// Description: Writes data to the LCD via I2C taking in a byte of data
void lcdWrite(uint8_t value) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(value);
  Wire.endTransmission();
}
