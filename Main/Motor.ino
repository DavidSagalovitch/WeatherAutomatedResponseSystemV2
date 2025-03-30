#include <Arduino.h>

// Motor control pins
#define MOTOR_PWM 16      // PWM pin for motor speed control
#define MOTOR_DIR 17      // Direction control pin

// Define speed scaling parameters
#define MIN_SPEED 50     // Minimum PWM speed
#define MAX_SPEED 255    // Maximum PWM speed

#define WIPER_CYCLES 1

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

int getMotorSpeed(int wiper_speed_ms) {
    if (wiper_speed_ms == 1800) return 15;
    else if (wiper_speed_ms == 800) return 30;
    else if (wiper_speed_ms == 475) return 45;
    else if (wiper_speed_ms == 500) return 60;
    else return 0;  //Max speed if it's below 300
    // speed = 15, wiper_speed_ms = 2000;
    // speed = 30, wiper_speed_ms = 800;
    // speed 45, wiper_speed_ms = 475;
    // speed 60, wiper_speed_ms = 300;
}

// Motor task (runs continuously)
void motor_task(void *pvParameters) {
    motor_init(); 
    int speed = 0;
    bool port = 1;
    while (true) {
      int wiper_speed_ms = getWiperSpeed();
      for (int i = 0; i < WIPER_CYCLES; i++){
          speed = getMotorSpeed(wiper_speed_ms);
          speed = 60;
          if (wiper_speed_ms>0) {
              //Serial.print("Setting motor speed to ");
              //Serial.print(speed);
              //Serial.print(" for 180-degree rotation in ");
              //Serial.print(wiper_speed_ms);
              //Serial.println(" ms");
              // Move left (reverse)
              if (port){
                  motor_spin_reverse(speed);
                  vTaskDelay(pdMS_TO_TICKS(350));  // Convert ms to RTOS ticks
                  motor_stop();
                  //vTaskDelay(pdMS_TO_TICKS(1000));
                  port = 0;
              }
              else{
              //Serial.println("Spinning Reverse");
                // Move right (forward)
                motor_spin_forward(speed);
                vTaskDelay(pdMS_TO_TICKS(350));
                motor_stop();
                //vTaskDelay(pdMS_TO_TICKS(1000));
                port = 1;
              }

              //Serial.println("Spinning Forward");
              wiper_at_rest.store(0, std::memory_order_relaxed);
          }

          vTaskDelay(pdMS_TO_TICKS(2500));  // Small delay to prevent watchdog reset
      }
      wiper_at_rest.store(1, std::memory_order_relaxed);
    }
}

