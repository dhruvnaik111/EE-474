#include <ESP32Servo.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>

// --- Pins ---
const int servoPin = 13;
const int IN1 = 19;
const int IN2 = 18;
const int IN3 = 5;
const int IN4 = 17;
const int trigPin = 4;
const int echoPin = 2;

// --- Objects ---
Servo myServo;
const int stepsPerRev = 2048; 
Stepper myStepper(stepsPerRev, IN1, IN3, IN2, IN4);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int currentPos = 0; // Track servo position

void setup() {
  Serial.begin(115200);

  // Sensor Pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // LCD Setup
  Wire.begin(8, 9); // Initialize I2C on pins 8 (SDA) and 9 (SCL) 
  lcd.init();       // 
  delay(2);         // 
  lcd.backlight();  // 
  lcd.clear();      // 

  // Servo Setup
  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(0); 

  // Stepper Setup - keep speed low to prevent heat
  myStepper.setSpeed(8); 
  
  // Start with all motor pins OFF
  releaseStepper();
  
  Serial.println("System initialized with Capacitor Buffer & Sensors.");
}

void loop() {
  // 0. Read Sensor and Update LCD
  float distance = getDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Update LCD without clearing the screen to prevent flickering
  lcd.setCursor(0, 0);       // [cite: 100]
  lcd.print("Dist: ");       
  lcd.print(distance);       
  lcd.print(" cm    ");      // Pad with spaces to clear old digits [cite: 101]

  // 1. Move Stepper 90 degrees (512 steps)
  Serial.println("Stepper Moving...");
  myStepper.step(512);
  releaseStepper(); // Turn off coils to save power
  delay(1000); 

  // 2. Move Servo slowly to 90
  Serial.println("Servo Moving Slowly...");s
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

// Function to measure distance in centimeters
float getDistance() {
  // Ensure the trigger pin is clear
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send a 10 microsecond pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the duration of the echo pulse
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  
  if (duration == 0) {
    return -1.0; // Return -1 if no ping received
  }
  
  // Calculate distance: (duration / 2) * speed of sound (0.0343 cm/us)
  float distance_cm = (duration / 2.0) * 0.0343;
  return distance_cm;
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