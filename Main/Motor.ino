#include <Arduino.h>

// Motor control pins
#define MOTOR_PWM 16      // PWM pin for motor speed control
#define MOTOR_DIR 17      // Direction control pin

// Define speed scaling parameters
#define MIN_SPEED 50     // Minimum PWM speed
#define MAX_SPEED 255    // Maximum PWM speed

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
    if (wiper_speed_ms == 250) return 60;
    else if (wiper_speed_ms == 200) return 90;
    else if (wiper_speed_ms == 150) return 120;
    else return 0;  //Max speed if it's below 300
    // speed = 60, wiper_speed_ms = 250;
    // speed 90, wiper_speed_ms = 200;
    // speed 120, wiper_speed_ms = 150;
}

// Motor task (runs continuously)
void motor_task(void *pvParameters) {
    motor_init(); 
    int speed = 0;
    bool port = 1;
    int wipes = 0;
    while (true) {
      int wiper_speed_ms = getWiperSpeed();
      speed = getMotorSpeed(wiper_speed_ms);
      if (wiper_speed_ms>0) {
            //Serial.print("Setting motor speed to ");
            //Serial.print(speed);
            //Serial.print(" for 180-degree rotation in ");
            //Serial.print(wiper_speed_ms);
            //Serial.println(" ms");
            //Move left (reverse);
            if (port){
                motor_spin_reverse(speed);
                vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));  // Convert ms to RTOS ticks
                motor_stop();
                //vTaskDelay(pdMS_TO_TICKS(1000));
                port = 0;
            }
            else{
            //Serial.println("Spinning Reverse");
              // Move right (forward)
              motor_spin_forward(speed);
              vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));
              motor_stop();
              //vTaskDelay(pdMS_TO_TICKS(1000));
              port = 1;
            }
            //Serial.println("Spinning Forward");
      }
      vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

