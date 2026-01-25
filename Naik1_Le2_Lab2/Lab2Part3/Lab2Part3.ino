// Filename: Lab2Part3.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/25/2026
// Description: Use a photoresistor to control the brightness of an LED

// ==================== Macros ====================
#define LED_PIN 5
#define PHOTORESISTOR_PIN 1

// ==================== Functions ====================
void setup(){
  Serial.begin(9600);
  ledcAttach(LED_PIN, 1000, 8);
}

void loop() {
  int lightLevel = analogRead(PHOTORESISTOR_PIN);
  int duty = map(lightLevel, 0, 4095, 0, 255);
  ledcWrite(LED_PIN, duty);
  delay(10);
}