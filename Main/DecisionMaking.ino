#include <atomic>

std::atomic<float> camera_rain_intensity(0);
std::atomic<uint16_t> camera_rain_detected(0);
std::atomic<bool> is_day(0);
std::atomic<bool> lidar_rain_detected(0);
std::atomic<bool> lidar_rain_intensity(0);

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
  if (intensity < 25.0f) return 1800;
  else if (intensity < 50.0f) return 800;
  else if (intensity < 80.0f) return 475;
  else return 300;


  // speed = 15, wiper_speed_ms = 2000;
  // speed = 30, wiper_speed_ms = 800;
  // speed 45, wiper_speed_ms = 475;
  // speed 60, wiper_speed_ms = 300;
  return 0;
}