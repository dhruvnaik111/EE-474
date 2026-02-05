// Name: icte6.ino
// Author: Ethan Le
// Date: 02/5/2026
// Description: Toggles an LED and prints a message every second using esp_timer.

#include "esp_timer.h"

// --- Define GPIO Pins --- //
#define LED_PIN     5
#define BUTTON_PIN  6

// --- Define Timer Interval --- //
#define TOGGLE_INTERVAL_US 1000000  // 1 second

// --- Global Variables --- //
esp_timer_handle_t periodic_timer;
volatile bool ledState = false;

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200;  // milliseconds

// --- TODO: Timer ISR --- //
void IRAM_ATTR onTimer(void* arg) {
  // TODO: Toggle LED state and print message
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.println("Toggled LED");
}

// --- TODO: Button ISR with Debounce --- //
void IRAM_ATTR handleButtonInterrupt() {
  // TODO: Check millis() and debounce, then print
  const unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime > debounceDelay) {
    Serial.println("Button Pressed!");
    lastInterruptTime = currentTime;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for Serial Monitor

  // TODO: Configure LED pin as OUTPUT and set LOW
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // TODO: Configure BUTTON pin as INPUT_PULLUP
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // TODO: Attach button interrupt on FALLING edge
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);

  // TODO: Create and start periodic esp_timer
  //       Attach onTimer as the callback
  //       Start it at 1-second interval
  esp_timer_create_args_t timerArgs = {
    .callback = &onTimer,
    .name = "LED Toggle Timer"
  };
  esp_timer_create(&timerArgs, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, TOGGLE_INTERVAL_US);
}

void loop() {
  // Nothing here – logic handled by interrupts
  delay(1000);
}