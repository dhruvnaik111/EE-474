// Filename:
// Authors:
// Date:
// Description:

// ==================== Defines ====================
#define LED_PIN 1
#define PHOTORES_PIN 5

void setup() {
  Serial.begin(9600);
  ledcAttach(LED_PIN, 1000, 8);
}

void loop() {
  int lightLevel = analogRead(PHOTORES_PIN);
  int duty = map(lightLevel, 0, 4095, 0, 255);
  ledcWrite(LED_PIN, duty);
  delay(10);
}
