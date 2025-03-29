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
#include <atomic>
#define OV5642_MINI_5MP_PLUS
#if !(defined(OV5642_MINI_5MP_PLUS))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
#define FRAMES_NUM 0x00
// set pin 7 as the slave select for the digital pot:
const int CS = 5;
#define SD_CS 9
bool is_header = false;
int total_time = 0;
//uint8_t resolution = OV5642_320x240;
uint8_t resolution = OV5642_1280x960;
uint32_t line, column;

std::atomic<bool> wiper_at_rest(0);
std::atomic<bool> wifi_connected(0); 
std::atomic<bool> client_connected(0);

void process_image();
void wifiSetup();
void sendPhotoOverWifi();

ArduCAM myCAM(OV5642, CS);

void initializeCameraForWifi() {
  Serial.println("📸 Initializing camera for WiFi mode (JPEG @ 1280x960)...");

  myCAM.set_format(JPEG);
  delay(100);
  myCAM.InitCAM();  // Good idea to reinit when changing modes
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.OV5642_set_JPEG_size(OV5642_1280x960);
  delay(100);

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);

  Serial.println("Camera ready for JPEG capture.");
}

void initializeCameraForLocalProcessing() {
  Serial.println("📸 Initializing camera for RAW mode (320x240)...");

  myCAM.set_format(RAW);
  delay(100);
  myCAM.InitCAM();  // Reinitialize
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.OV5642_set_RAW_size(OV5642_320x240);
  delay(100);

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);

  Serial.println("Camera ready for RAW grayscale capture.");
}



void cameraTask(void *pvParameters)
{
// put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;
  wifiSetup();

  Wire.begin();
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
  // Change to JPEG capture mode and initialize the OV5640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.OV5642_set_JPEG_size(resolution);
  delay(1000);
  while(1)
  { 
    if(wiper_at_rest.load(std::memory_order_relaxed))
    {
      char VL;
      byte buf[256];  // Temporary buffer for reading data
      int m = 0;
      uint32_t line, column;
      
      // Flush and clear the FIFO
      myCAM.flush_fifo();
      myCAM.clear_fifo_flag();
      myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
      // Set resolution (modify this as needed)
      if (wifi_connected.load(std::memory_order_relaxed) && client_connected.load(std::memory_order_relaxed)) {
          initializeCameraForWifi();
          delay(1000);  // Give time for settings to apply
      } else {
          initializeCameraForLocalProcessing();
          delay(1000);  // Give time for settings to apply
      }


      // Start capture
      myCAM.start_capture();
      Serial.println(F("Start capture..."));
      
      // Wait until capture is done
      while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
        yield();  // Prevent watchdog reset
      }
      Serial.println(F("Capture done!"));
      if (wifi_connected.load(std::memory_order_relaxed) && client_connected.load(std::memory_order_relaxed)) {
        sendPhotoOverWifi();
      }
      else{
      Serial.print("FIFO first 10 bytes: ");
        for (int i = 0; i < 10; i++) {
            uint8_t val = myCAM.read_fifo();
            Serial.print(val, HEX);
            Serial.print(" ");
        }
        Serial.println();
        process_image();
        sendPhotoOverWifi();
      }

      myCAM.flush_fifo();
      myCAM.clear_fifo_flag();  // Clear the capture flag

    }
  }
}
