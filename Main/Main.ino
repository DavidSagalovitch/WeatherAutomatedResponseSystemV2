// ArduCAM demo (C)2017 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to capture image in RAW format
// 1. Capture and buffer the image to FIFO every 5 seconds
// 2. Store the image to Micro SD/TF card with RAW format.
// 3. You can change the resolution by change the "resolution = OV5642_640x480"
// This program requires the ArduCAM V4.0.0 (or above) library and ArduCAM shield V2
// and use Arduino IDE 1.6.8 compiler or above
#include <Wire.h>
#include "../ArduCAM.h"  
#include <SPI.h>
#define OV5642_MINI_5MP_PLUS
//#include "../src/memorysaver.h"
// This demo can only work on OV5640_MINI_5MP_PLUS or OV5642_MINI_5MP_PLUS platform.
#if !(defined(OV5642_MINI_5MP_PLUS))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define FRAMES_NUM 0x00
// set pin 7 as the slave select for the digital pot:
const int CS = 5;
#define SD_CS 9
bool is_header = false;
int total_time = 0;
uint8_t resolution = OV5642_640x480;
uint32_t line, column;
ArduCAM myCAM(OV5642, CS);
uint8_t saveRAW(void);
// Task Handles
TaskHandle_t TaskCamera;

void cameraTask(void *pvParameters)
{
// put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;

#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.println(F("ArduCAM Start!"));
  // set the CS as an output:4096
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  // initialize SPI:
  SPI.begin();
  // Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while (1)
  {
    // Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
    {
      Serial.println(F("SPI interface Error!"));
      delay(1000);
      continue;
    }
    else
    {
      Serial.println(F("SPI interface OK."));
      break;
    }
  }

  while (1)
  {
    // Check if the camera module type is OV5642
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if ((vid != 0x56) || (pid != 0x42))
    {
      Serial.println(F("Can't find OV5642 module!"));
      delay(1000);
      continue;
    }
    else
    {
      Serial.println(F("OV5642 detected."));
      break;
    }
  }
  // Change to JPEG capture mode and initialize the OV5642 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);

  while(1)
  { 
    char VL;
    byte buf[256];  // Temporary buffer for reading data
    int m = 0;
    uint32_t line, column;
    
    // Flush and clear the FIFO
    myCAM.flush_fifo();
    myCAM.clear_fifo_flag();
    
    // Set resolution (modify this as needed)
    myCAM.OV5642_set_RAW_size(resolution);
    delay(1000);  // Give time for settings to apply

    // Start capture
    myCAM.start_capture();
    Serial.println(F("Start capture..."));
    
    // Wait until capture is done
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
      yield();  // Prevent watchdog reset
    }
    Serial.println(F("Capture done!"));

  // Determine resolution settings (for looping purposes)
  if (resolution == OV5642_320x240) {
        line = 320;
        column = 240;
  } else if (resolution == OV5642_640x480) {
    line = 640;
    column = 480;
  } else if (resolution == OV5642_1280x960) {
    line = 1280;
    column = 960;
  } else if (resolution == OV5642_1920x1080) {
    line = 1920;
    column = 1080;
  } else if (resolution == OV5642_2592x1944) {
    line = 2592;
    column = 1944;
  }

    Serial.println(F("Printing image bytes (RAW format):"));

    Serial.println(F("IMAGE_START"));  // Signal start of image data

    for (uint32_t i = 0; i < line; i++) {
      for (uint32_t j = 0; j < column; j++) {
        VL = myCAM.read_fifo();  // Read one byte from the camera FIFO
        Serial.printf("%02X ", VL);  // Print byte in hexadecimal format

        // Optional: Print 16 bytes per line for readability (can be removed if needed)
        if ((i * column + j + 1) % 16 == 0) {
        Serial.println();
        }
      }
    }

    Serial.println(F("\nIMAGE_END"));  // Signal end of image data
    myCAM.clear_fifo_flag();  // Clear the capture flag

    delay(5000);  // Wait before capturing again


    Serial.println(F("\nImage print complete."));
    myCAM.clear_fifo_flag();  // Clear the capture flag

    delay(5000);  // Wait before capturing again}
  }
}

#include <Arduino.h>

// Motor control pins
#define MOTOR_PWM 16      // PWM pin for motor speed control
#define MOTOR_DIR 17      // Direction control pin

// Task handle
TaskHandle_t TaskMotor = NULL;

// Define speed scaling parameters
#define MIN_SPEED 50     // Minimum PWM speed
#define MAX_SPEED 255    // Maximum PWM speed
#define DEFAULT_SPEED 116

// Variables
bool rain_detected = true;    // Simulated rain detection
int wiper_speed_ms = 500;     // Time for 180-degree movement in ms

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
    motor_init();  // Initialize motor
    while (true) {
        if (rain_detected) {
            if (wiper_speed_ms < 116) {
                wiper_speed_ms = 116;  // Prevent too fast motion
            }

            int speed = map(wiper_speed_ms, 1000, 116, MIN_SPEED, MAX_SPEED);
            speed = constrain(speed, MIN_SPEED, MAX_SPEED);  // Limit speed

            Serial.print("Setting motor speed to ");
            Serial.print(speed);
            Serial.print(" for 180-degree rotation in ");
            Serial.print(wiper_speed_ms);
            Serial.println(" ms");

            // Move left (reverse)
            motor_spin_reverse(speed);
            vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));  // Convert ms to RTOS ticks
            motor_stop();
            vTaskDelay(pdMS_TO_TICKS(500));

            Serial.println("Spinning Reverse");

            // Move right (forward)
            motor_spin_forward(speed);
            vTaskDelay(pdMS_TO_TICKS(wiper_speed_ms));
            motor_stop();

            Serial.println("Spinning Forward");
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to prevent watchdog reset
    }
}


void setup(){
    Serial.begin(115200);
    xTaskCreatePinnedToCore(cameraTask, "CameraTask", 8192, NULL, 1, &TaskCamera, 0);
        // Create Motor Task on Core 0
   xTaskCreatePinnedToCore(
      motor_task, "MotorTask",4096, NULL, 1, &TaskMotor,0);
      
     }

void loop() {
   vTaskDelay(1000 / portTICK_PERIOD_MS);
}
