#include <Arduino.h>

// Motor control pins
#define MOTOR_PWM 16      // PWM pin for motor speed control
#define MOTOR_DIR 17      // Direction control pin

// Define speed scaling parameters
#define MIN_SPEED 50     // Minimum PWM speed
#define MAX_SPEED 255    // Maximum PWM speed

#define WIPER_CYCLES 7

uint16_t getWiperSpeed();


// Motor initialization
void motor_init() {
    pinMode(MOTOR_PWM, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    digitalWrite(MOTOR_DIR, LOW);
    analogWrite(MOTOR_PWM, 0);
}

// Motor forward
void motor_spin_forward(int speed) {
    digitalWrite(MOTOR_DIR, HIGH);  // Set direction forward
    analogWrite(MOTOR_PWM, speed);  // Set speed
    Serial.print("Motor spinning forward at speed: ");
    Serial.println(speed);
}

// Motor reverse
void motor_spin_reverse(int speed) {
    digitalWrite(MOTOR_DIR, LOW);  // Set direction reverse
    analogWrite(MOTOR_PWM, speed); // Set speed
    Serial.print("Motor spinning reverse at speed: ");
    Serial.println(speed);
}

// Motor stop
void motor_stop() {
    analogWrite(MOTOR_PWM, 0);  // Stop motor
    Serial.println("Motor stopped");
}

// Motor task (runs continuously)
void motor_task(void *pvParameters) {
    motor_init(); 
    while (true) {
      int wiper_speed_ms = getWiperSpeed();
      for (int i = 0; i < WIPER_CYCLES; i++){
          if (wiper_speed_ms>0) {
              if (wiper_speed_ms < 116) {
                  wiper_speed_ms = 116;  // Prevent too fast motion
              }

              int speed = map(wiper_speed_ms, 1500, 116, MIN_SPEED, MAX_SPEED);
              speed = constrain(speed, MIN_SPEED, MAX_SPEED);  // Limit speed

              //Serial.print("Setting motor speed to ");
              //Serial.print(speed);
              //Serial.print(" for 180-degree rotation in ");
              //Serial.print(wiper_speed_ms);
              //Serial.println(" ms");

              // Move left (reverse)
              motor_spin_reverse(speed);
              vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));  // Convert ms to RTOS ticks
              motor_stop();
              vTaskDelay(pdMS_TO_TICKS(500));

              //Serial.println("Spinning Reverse");

              // Move right (forward)
              motor_spin_forward(speed);
              vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));
              motor_stop();

              //Serial.println("Spinning Forward");
              wiper_at_rest.store(0, std::memory_order_relaxed);
          }

          vTaskDelay(pdMS_TO_TICKS(150));  // Small delay to prevent watchdog reset
          wiper_at_rest.store(1, std::memory_order_relaxed);
      }
    }
}

