#include <ESP32Servo.h>
#include <Stepper.h>

// --- Pins ---
const int servoPin = 13;
const int IN1 = 19;
const int IN2 = 18;
const int IN3 = 5;
const int IN4 = 17;

// --- Objects ---
Servo myServo;
const int stepsPerRev = 2048; 
Stepper myStepper(stepsPerRev, IN1, IN3, IN2, IN4);

int currentPos = 0; // Track servo position

void setup() {
  Serial.begin(115200);

  // Servo Setup
  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(0); 

  // Stepper Setup - keep speed low to prevent heat
  myStepper.setSpeed(8); 
  
  // Start with all motor pins OFF
  releaseStepper();
  
  Serial.println("System initialized with Capacitor Buffer.");
}

void loop() {
  // 1. Move Stepper 90 degrees (512 steps)
  Serial.println("Stepper Moving...");
  myStepper.step(512);
  releaseStepper(); // Turn off coils to save power
  delay(1000); 

  // 2. Move Servo slowly to 90
  Serial.println("Servo Moving Slowly...");
  moveServoGentle(90);
  delay(1000);

  // 3. Reset everything
  moveServoGentle(0);
  delay(500);
  myStepper.step(-512);
  releaseStepper();

  Serial.println("Cooldown period...");
  delay(4000); // 4-second rest to let the regulator stay cool
}

void moveServoGentle(int target) {
  int stepDir = (target > currentPos) ? 1 : -1;
  
  while (currentPos != target) {
    currentPos += stepDir;
    myServo.write(currentPos);
    delay(30); // Higher delay = less current draw
  }
}

void releaseStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}