TaskHandle_t TaskCamera; // Declare task handle
TaskHandle_t TaskMotor;  // Declare task handle
TaskHandle_t TaskLidar;  // Declare task handle

void setup(){
    Serial.begin(115200);
    xTaskCreatePinnedToCore(cameraTask, "CameraTask", 8192, NULL, 1, &TaskCamera, 0);
    xTaskCreatePinnedToCore(motor_task, "MotorTask", 4096, NULL, 1, &TaskMotor, 0);
    xTaskCreatePinnedToCore(lidarTask, "LidarTask", 4096, NULL, 1, &TaskLidar, 0);
    }

void loop() {
   vTaskDelay(1000 / portTICK_PERIOD_MS);
}
