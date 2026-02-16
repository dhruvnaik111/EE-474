#include <stdarg.h>

extern "C" void hw_printf(const char* format, ...) {
  char buffer[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  Serial.print(buffer);
}

extern "C" {
  void run_array_ops();
  void run_array_2d();
  void run_sensor_buffer();
}

void setup() {
    Serial.begin(9600);
    delay(5000);
    while(!Serial);
    Serial.println("======== PART I ========");
    run_array_ops();

    Serial.println("======== PART II ========");
    run_array_2d();

    Serial.println("======== PART III ========");
    run_sensor_buffer();
}

void loop() {}
