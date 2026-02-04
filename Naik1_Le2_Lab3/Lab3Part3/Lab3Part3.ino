// Filename: Lab3Part3.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 02/07/2026
// Description: Introduce the concept of interrupts and ISRs to interact with the LCD and BLE communication.

// ==================== Includes ====================
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <LiquidCrystal_I2C.h>

// ==================== Macros ====================
//===============> TODO:
// Generate random Service and Characteristic UUIDs: https://www.uuidgenerator.net/
#define SERVICE_UUID        "d28fb83d-8341-4198-8a40-8564a59ef11d"
#define CHARACTERISTIC_UUID "64a5d168-17ce-45af-bc1c-7ba2ef5b5105"

#define BUTTON_PIN 2
hw_timer_t * timer = NULL;

// ==================== Global Variables ====================
volatile int counter = 0;
volatile bool buttonPressed = false;
volatile bool newMessage = false;
volatile bool iscounting = true;
volatile bool timerFired = false;

const unsigned long BUTTON_LOCKOUT_MS = 2500;
unsigned long lastButtonHandled = 0;


// ==================== Classes ====================
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // =========> TODO: This callback function will be invoked when signal is
    // 		     received over BLE. Implement the necessary functionality that
    //		     will trigger the message to the LCD.

    newMessage = true;
    iscounting = false;
  }
};

// ==================== Functions ====================
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD

// ==============> TODO: Write your timer ISR here.
void IRAM_ATTR timerInterrupt() {
  // Minimal ISR: set a flag for the main loop to handle the counting
  timerFired = true;
} 

// ==============> TODO: Create an ISR function to handle button press here.
void IRAM_ATTR buttonInterrupt() {
  // Set button pressed flag only (keep ISR minimal)
  buttonPressed = true;
} 

void setup() {
  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                      );


  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();


  // =========> TODO: Initialize LCD display
  Wire.begin(8,9);
  lcd.init();
  delay(2);
  lcd.backlight();
  lcd.clear();

  // =========> TODO: create a timer, attach an interrupt, set an alarm which will
  //                  update the counter every second.
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timerInterrupt);
  timerAlarm(timer, 1000000, true, 0); // 1 second interval
  timerStart(timer);
  // ========> TODO: Set button pin as input and attach an interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP);  
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, FALLING);                                  
}


void loop() {
  // =========> TODO: Print out an incrementing counter to the LCD.
  //                  If a signal has been received over BLE, print out “New
  //                  Message!” on the LCD.
  //                  If the button has been pressed, print out "Button Pressed"
  //                  on the LCD.

  // update counter only when counting is enabled and clear flag
  if (timerFired) {
    if (iscounting) {
      counter++;
    }
    timerFired = false;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Count: ");
  lcd.print(counter);

  if (buttonPressed) {
    // check for lockout time
    if (millis() - lastButtonHandled < BUTTON_LOCKOUT_MS) {
      buttonPressed = false;
    } else {
      iscounting = false; // pause counting while handling press
      lcd.setCursor(0, 1);
      lcd.print("Button Pressed");
      buttonPressed = false;
      lastButtonHandled = millis();
      delay(2000); // 2 second delay to show message
      iscounting = true;
    }
  }

  if(newMessage) {
    lcd.setCursor(0, 1);
    lcd.print("New Message!");
    newMessage = false;
    delay(2000); // 2 second delay to show message
    iscounting = true;
  }
  
  delay(100);
}
