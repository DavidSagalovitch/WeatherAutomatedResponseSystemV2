#include <atomic>

std::atomic<float> camera_rain_intensity(0);
std::atomic<uint16_t> camera_rain_detected(0);
std::atomic<bool> is_day(0);
std::atomic<bool> lidar_rain_detected(0);
std::atomic<bool> lidar_wiper_detected(0);
std::atomic<bool> lidar_rain_intensity(0);
std::atomic<bool> camera_fog(0);

uint16_t getWiperSpeed()
{
  bool day = is_day.load(std::memory_order_relaxed);
  float intensity = camera_rain_intensity.load(std::memory_order_relaxed);
/*
  if(day)
  {

  }
  else{
    
  }
  */
  Serial.print("Current rain intensity: ");
  Serial.println(intensity);

  if (intensity < 7.0f) return 0;
  if (intensity < 25.0f) return 250;
  else if (intensity < 50.0f) return 200;
  else return 150;
  
  return 0;
}

uint16_t getFanSpeed()
{
  bool fog = camera_fog.load(std::memory_order_relaxed);
  if (fog != 0) {
    uint16_t fan_speed = 255;
    return fan_speed;
  }
  else {
    return 0;
  }
}