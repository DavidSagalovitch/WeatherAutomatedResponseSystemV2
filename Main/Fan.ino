#include <Arduino.h>
#define FAN_PWM 12      // PWM control pin
#define FAN_TACH 13     // Fan tach input pin
#define FAN_PWM_FREQ 25000  // Typical fan PWM frequency
#define FAN_PWM_CHANNEL 0   // PWM channel
#define FAN_PWM_RESOLUTION 8
#define TACH_PULSES_PER_REV 2  // Most PC fans give 2 pulses per revolution

volatile unsigned long tach_pulse_count = 0;
unsigned long last_rpm_check_time = 0;
int rpm = 0;

void IRAM_ATTR tachISR() {
  tach_pulse_count++;
}

void fan_init() {
  analogWriteFrequency(FAN_PWM, FAN_PWM_FREQ);
  pinMode(FAN_PWM, OUTPUT);
  // Set initial fan speed to OFF (0% duty)
  analogWrite(FAN_PWM, 0);

  // Set up tach input on GPIO 13
  pinMode(FAN_TACH, INPUT);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH), tachISR, FALLING);

  Serial.println("Fan control initialized.");
}

void printFanRPM() {
  unsigned long start_pulse_count = tach_pulse_count;
  unsigned long start_time = millis();

  delay(1000);  // Measure for 1 second

  unsigned long end_pulse_count = tach_pulse_count;
  unsigned long end_time = millis();

  unsigned long pulse_diff = end_pulse_count - start_pulse_count;
  float time_diff = (end_time - start_time) / 1000.0; // seconds

  rpm = (pulse_diff / (float)TACH_PULSES_PER_REV) / time_diff * 60.0;
  Serial.print("Fan RPM: ");
  Serial.println(rpm);
}

void fanTask(void *pvParameters) {
  while (true){
     Serial.println("Increasing fan speed...");
  
    // Gradually increase speed
    for (int duty = 0; duty <= 255; duty += 50) {
      analogWrite(FAN_PWM_CHANNEL, duty);
      Serial.print("Set fan speed: ");
      Serial.print((duty / 255.0) * 100);
      Serial.println("%");
      delay(2000);
      printFanRPM();
    }

    Serial.println("Decreasing fan speed...");
    
    // Gradually decrease speed
    for (int duty = 255; duty >= 0; duty -= 50) {
      analogWrite(FAN_PWM_CHANNEL, duty);
      Serial.print("Set fan speed: ");
      Serial.print((duty / 255.0) * 100);
      Serial.println("%");
      delay(2000);
      printFanRPM();
    }
    Serial.println("Turning fan off...");
    analogWrite(FAN_PWM_CHANNEL, 0); // Turn fan off
    delay(3000);
  }
}