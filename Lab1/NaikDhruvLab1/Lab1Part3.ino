// Filename: Lab1Part3.ino
// Author: Dhruv Naik
// Date: 01/12/2026
// Description: This file controls an LEDs on an ESP32 when pressed by a pushbutton

// ============================= Constants =======================================
const int LED1 = 19; //LED1
const int LED2 = 21; //LED2

const int BTN1 = 4;   //Button 1
const int BTN2 = 5;   //Button 2

// ============================= Global Variables =======================================
bool led1Flashing = true; //true if LED1 is flashing
bool led2Flashing = true; //true if LED2 is flashing

// LED state i.e. whether the LED is on or off
bool led1State = false;
bool led2State = false;

// The last time the LEDs changed state
unsigned long previousMillis1 = 0; 
unsigned long previousMillis2 = 0;

//time in between LED flashing
const unsigned long flashInterval = 500; 

//button states
bool btn1Prev = HIGH;
bool btn2Prev = HIGH;

//the last time the button was toggled
unsigned long btn1LastChange = 0;
unsigned long btn2LastChange = 0;

const unsigned long debounceDelay = 25; //How long to check for debounce

// ============================= Setup =======================================
void setup() {
  //Set LEDs to output
  pinMode(LED1, OUTPUT); 
  pinMode(LED2, OUTPUT); 

  //Set buttons to input (when pressed state is LOW)
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  //Write LEDs to LOW
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
}

// ============================= Loop =======================================
void loop() {
  //what the current time is now
  unsigned long now = millis();


  // BUTTON 1 logic ---------------------------------
  int reading1 = digitalRead(BTN1);

  //if the button state changed
  if (reading1 != btn1Prev) {
    btn1LastChange = now; //record the time the state changed
    btn1Prev = reading1; //record the state as previous
  }

  //if button is pressed AND its been there for longer than the debounce
  if (reading1 == LOW && (now - btn1LastChange) > debounceDelay) {
    led1Flashing = !led1Flashing; //togle if the led is flashing
    if (!led1Flashing) digitalWrite(LED1, LOW); //if it is not flashing turn the LED off
    
    //While button is pressed - prevents the button press being recorded for multiple cycles
    while (digitalRead(BTN1) == LOW) {
      //do nothing
    }
    delay(10); //delay to prevent multiple presses
  }

  // BUTTON 2 logic (same logic as button 1)---------------------------------
  int reading2 = digitalRead(BTN2);

  if (reading2 != btn2Prev) {
    btn2LastChange = now;
    btn2Prev = reading2;
  }

  if (reading2 == LOW && (now - btn2LastChange) > debounceDelay) {
    led2Flashing = !led2Flashing;
    if (!led2Flashing) digitalWrite(LED2, LOW);

    while (digitalRead(BTN2) == LOW) {}
    delay(10);
  }


  // LED 1 flashing logic ---------------------------------
  //if the led is flashing and the time elapsed is greater than the flash interval
  if (led1Flashing && now - previousMillis1 >= flashInterval) {
    previousMillis1 = now; //set the time the state changed
    led1State = !led1State; //flip LED state
    digitalWrite(LED1, led1State); //write the new state to pin
  }

  // LED 2 flashing logic (same logic as led 1) ---------------------------------
  if (led2Flashing && now - previousMillis2 >= flashInterval) {
    previousMillis2 = now;
    led2State = !led2State;
    digitalWrite(LED2, led2State);
  }
}