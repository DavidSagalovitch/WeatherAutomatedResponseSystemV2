#define FAN_PWM_PIN 12      // PWM control pin
#define FAN_TACH_PIN 13     // Fan tach input pin
#define FAN_PWM_FREQ 25000  // Typical fan PWM frequency
#define FAN_PWM_RESOLUTION 8
#define TACH_PULSES_PER_REV 2  // Most PC fans give 2 pulses per revolution

volatile unsigned long tach_pulse_count = 0;
unsigned long last_rpm_check_time = 0;
int rpm = 0;

void IRAM_ATTR tachISR() {
  tach_pulse_count++;
}

void setup() {
  // Set up PWM on GPIO 12
  ledcSetup(0, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);  
  ledcAttachPin(FAN_PWM_PIN, 0);

  // Set up tach input on GPIO 13
  pinMode(FAN_TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACH_PIN), tachISR, FALLING);

  Serial.println("Fan control with RPM monitoring started.");
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

void fanTask() {
  while (true){
    // Ramp fan speed from 0 to 100%
    for (int duty = 0; duty <= 255; duty += 50) {
      ledcWrite(0, duty);
      Serial.print("Set fan duty cycle: ");
      Serial.println(duty);
      delay(2000);
      printFanRPM();
    }

    // Ramp down from 100% to 0%
    for (int duty = 255; duty >= 0; duty -= 50) {
      ledcWrite(0, duty);
      Serial.print("Set fan duty cycle: ");
      Serial.println(duty);
      delay(2000);
      printFanRPM();
    }

    delay(3000);
  }
}