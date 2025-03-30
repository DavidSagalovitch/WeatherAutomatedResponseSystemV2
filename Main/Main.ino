TaskHandle_t TaskCamera; // Declare task handle
TaskHandle_t TaskMotor;  // Declare task handle
TaskHandle_t TaskLidar;  // Declare task handle
TaskHandle_t TaskFan;  // Declare task handle


void setup(){
    Serial.begin(1000000);
    //xTaskCreatePinnedToCore(cameraTask, "CameraTask", 8192, NULL, 1, &TaskCamera, 1);
    //xTaskCreatePinnedToCore(motor_task, "MotorTask", 8192, NULL, 1, &TaskMotor, 0);
    //xTaskCreatePinnedToCore(lidarTask, "LidarTask", 4096, NULL, 1, &TaskLidar, 0);
    xTaskCreatePinnedToCore(fantask, "FanTask", 4096, NULL, 1, &TaskFan, 0);
    }

void loop() {
   vTaskDelay(1000 / portTICK_PERIOD_MS);
}
