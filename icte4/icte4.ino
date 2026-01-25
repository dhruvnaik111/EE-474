// Filename: icte4.ino
// Authors: Dhruv Naik, Ethan Le
// Date: 01/22/2026
// Description: Practice using a Round Robin Scheduler

// ==================== Macros ====================
#define led1 5
#define led2 8
#define POTENTIOMETER_PIN 2

unsigned long startMillis1;
unsigned long currentMillis1;
unsigned long startMillis2;
unsigned long currentMillis2;
unsigned long startMillis3;
unsigned long currentMillis3;
const unsigned long period1 = 500;
const unsigned long period2 = 1000;
const unsigned long period3 = 2000;

void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  startMillis1 = millis(); //For task A
  startMillis2 = millis(); //For task B
  startMillis3 = millis(); //For task C
  Serial.begin(115200);
  Serial.println("Start");
}

void loop() {
  currentMillis1 = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  if (currentMillis1 - startMillis1 >= period1)
  {
    TaskA();
    startMillis1 = currentMillis1;
  }
  if (currentMillis2 - startMillis2 >= period2)
  {
    TaskB();
    startMillis2 = currentMillis2;
  }
  if (currentMillis3 - startMillis3 >= period3)
  {
    TaskC();
    startMillis3 = currentMillis3;
  }
}

void TaskA() {
 digitalWrite(led1, !digitalRead(led1)); //Toggle LED1
}

void TaskB() {
  digitalWrite(led2, !digitalRead(led2)); //Toggle LED2
}

void TaskC() {
  int sensorValue = analogRead(POTENTIOMETER_PIN); 
  Serial.println(sensorValue); //Print Potentiometer value
}
