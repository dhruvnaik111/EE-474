// Filename: Lab3Part1.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/07/2026
// Description: Display a message on a LCD device entered through the Serial Monitor.

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

// =================== LCD Helper Function Prototypes ====================
void lcdWrite(uint8_t value);
void lcdPulseEnable(uint8_t data);
void lcdWrite4bits(uint8_t nibble, uint8_t mode);
void lcdSend(uint8_t value, uint8_t mode);
void lcdCommand(uint8_t cmd);
void lcdData(uint8_t data);
void lcdClear();
void lcdSetCursor(uint8_t col, uint8_t row);
void lcdPrint(const String& s);

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

    // Read input from Serial Monitor
    if(Serial.available() > 0) {
        char c = Serial.read();
        if (c != '\n') {
            text += c;
        } else {
            displaytext = text;
            text = "";            

            // Display the text on the LCD
            Serial.println("Displaying on LCD: " + displaytext);
            lcdClear();
            lcdSetCursor(0, 0);
            lcdPrint(displaytext);
        }
   
    }
 
}

// =================== LCD Helper Functions ====================
// Name: lcdWrite
// Description: Writes data to the LCD via I2C taking in a byte of data
void lcdWrite(uint8_t value) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(value);
  Wire.endTransmission();
}

// Name: lcdPulseEnable
// Description: Pulses the Enable pin to latch data into the LCD
void lcdPulseEnable(uint8_t data) {
  lcdWrite(data | Enable | Backlight);
  delayMicroseconds(1);
  lcdWrite((data & ~Enable) | Backlight);
  delayMicroseconds(50);
}

// Name: lcdWrite4bits
// Description: Writes 4 bits of data to the LCD
void lcdWrite4bits(uint8_t nibble, uint8_t mode) {
  uint8_t data = (nibble & 0xF0) | mode | Backlight;
  lcdPulseEnable(data);
}

// Name: lcdSend
// Description: Sends a byte of data to the LCD in two 4-bit operations
void lcdSend(uint8_t value, uint8_t mode) {
  lcdWrite4bits(value & 0xF0, mode);
  lcdWrite4bits((value << 4) & 0xF0, mode);
}

// Name: lcdCommand
// Description: Sends a command byte to the LCD
void lcdCommand(uint8_t cmd) {
  lcdSend(cmd, 0x00);
  if (cmd == 0x01 || cmd == 0x02) delay(2); // clear/home need extra time
}

// Name: lcdData
// Description: Sends a data byte to the LCD
void lcdData(uint8_t data) {
  lcdSend(data, RegSel);
}

// Name: lcdClear
// Description: Clears the LCD display
void lcdClear() {
  lcdCommand(0x01);
}

// Name: lcdSetCursor
// Description: Sets the cursor position on the LCD
void lcdSetCursor(uint8_t col, uint8_t row) {
  static const uint8_t rowOffsets[] = {0x00, 0x40, 0x14, 0x54};
  lcdCommand(0x80 | (col + rowOffsets[row]));
}

// Name: lcdPrint
// Description: Prints a string to the LCD
void lcdPrint(const String& s) {
  for (size_t i = 0; i < s.length(); i++) {
    lcdData((uint8_t)s[i]);
  }
}