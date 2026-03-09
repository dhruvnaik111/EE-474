#include <ESP32Servo.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include <driver/i2s.h>
#include <math.h>

// --- Pins ---
const int servoPin = 13;
const int IN1 = 19;
const int IN2 = 18;
const int IN3 = 5;
const int IN4 = 17;
const int trigPin = 4;
const int echoPin = 2;

// --- I2S Microphone Pins (INMP441) ---
#define I2S_WS  42
#define I2S_SCK 41
#define I2S_SD  6

#define I2S_PORT          I2S_NUM_0
#define I2S_SAMPLE_RATE   16000
#define I2S_BUFFER_LEN    64

// --- Microphone Globals ---
float smoothedPeak = 0;
float noiseFloor = 0;
bool noiseCalibrated = false;
unsigned long lastCalibrationTime = 0;
#define CALIBRATION_INTERVAL_MS    10000
#define VOICE_THRESHOLD_MULTIPLIER 2.5
#define SPEECH_FREQ_WEIGHT         1.4

// --- Objects ---
Servo myServo;
const int stepsPerRev = 2048;
Stepper myStepper(stepsPerRev, IN1, IN3, IN2, IN4);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int currentPos = 0;

// --- I2S Setup ---
void setupMicrophone() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = I2S_BUFFER_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);

  Serial.println("Microphone initialized.");
}

// --- Noise Floor Calibration ---
void calibrateNoiseFloor() {
  Serial.println("Calibrating noise floor... please be quiet.");

  int32_t samples[I2S_BUFFER_LEN];
  size_t bytesRead = 0;
  double sum = 0;
  int totalSamples = 0;

  // Average RMS over 20 buffers (~1 second of audio)
  for (int b = 0; b < 20; b++) {
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytesRead, portMAX_DELAY);
    int count = bytesRead / sizeof(int32_t);
    for (int i = 0; i < count; i++) {
      int32_t s = samples[i] >> 8;
      sum += (double)s * s;
    }
    totalSamples += count;
  }

  noiseFloor = sqrt(sum / totalSamples);
  noiseCalibrated = true;
  lastCalibrationTime = millis();

  Serial.print("Noise floor calibrated: ");
  Serial.println(noiseFloor);
}

// --- Read Microphone ---
void readMicrophone() {
  int32_t samples[I2S_BUFFER_LEN];
  size_t bytesRead = 0;

  // Auto-calibrate noise floor on first run and every CALIBRATION_INTERVAL_MS
  if (!noiseCalibrated ||
      (millis() - lastCalibrationTime > CALIBRATION_INTERVAL_MS)) {
    calibrateNoiseFloor();
  }

  i2s_read(I2S_PORT, &samples, sizeof(samples), &bytesRead, portMAX_DELAY);
  int samplesRead = bytesRead / sizeof(int32_t);

  // --- RMS of current buffer ---
  double sum = 0;
  for (int i = 0; i < samplesRead; i++) {
    int32_t s = samples[i] >> 8;
    sum += (double)s * s;
  }
  float rms = sqrt(sum / samplesRead);

  // --- Noise-subtracted RMS ---
  float cleanRMS = 0;
  if (rms > noiseFloor) {
    cleanRMS = sqrt((rms * rms) - (noiseFloor * noiseFloor));
  }

  // --- Smooth the clean RMS to reduce jitter ---
  smoothedPeak = (0.3 * cleanRMS) + (0.7 * smoothedPeak);

  // --- Convert to dB ---
  float rawDB   = (rms > 0)      ? 20.0 * log10(rms / 8388608.0)      : -100.0;
  float cleanDB = (cleanRMS > 0) ? 20.0 * log10(cleanRMS / 8388608.0) : -100.0;

  // --- Voice detection ---
  float voiceThreshold = noiseFloor * VOICE_THRESHOLD_MULTIPLIER;
  bool voiceDetected = (rms > voiceThreshold);

  // --- Serial output ---
  Serial.print("Raw dB: ");         Serial.print(rawDB, 1);
  Serial.print(" | Clean dB: ");    Serial.print(cleanDB, 1);
  Serial.print(" | Noise floor: "); Serial.print(noiseFloor, 0);
  Serial.print(" | Voice: ");       Serial.println(voiceDetected ? "YES" : "no");

  // --- LCD: show clean dB and voice status ---
  lcd.setCursor(0, 1);
  if (voiceDetected) {
    lcd.print("Voice! ");
    lcd.print(cleanDB, 1);
    lcd.print("dB  ");
  } else {
    lcd.print("Quiet ");
    lcd.print(rawDB, 1);
    lcd.print("dB  ");
  }
}

// --- Distance Sensor ---
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) return -1.0;

  return (duration / 2.0) * 0.0343;
}

// --- Servo Helper ---
void moveServoGentle(int target) {
  int stepDir = (target > currentPos) ? 1 : -1;
  while (currentPos != target) {
    currentPos += stepDir;
    myServo.write(currentPos);
    delay(30);
  }
}

// --- Stepper Helper ---
void releaseStepper() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ===========================
void setup() {
  Serial.begin(115200);

  // Sensor Pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // LCD Setup
  Wire.begin(8, 9);
  lcd.init();
  delay(2);
  lcd.backlight();
  lcd.clear();

  // Servo Setup
  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(0);

  // Stepper Setup
  myStepper.setSpeed(8);
  releaseStepper();

  // Microphone Setup
  setupMicrophone();
  calibrateNoiseFloor(); // Run once at startup

  Serial.println("System initialized with Capacitor Buffer & Sensors.");
}

void loop() {
  // 0. Read Distance and Update LCD row 0
  float distance = getDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print(" cm    ");

  // 0b. Read Microphone and update LCD row 1
  readMicrophone();

  // 1. Move Stepper 90 degrees (512 steps)
  Serial.println("Stepper Moving...");
  myStepper.step(512);
  releaseStepper();
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
  delay(4000);
}