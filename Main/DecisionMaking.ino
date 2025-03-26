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

  if (intensity < 10.0f) return 0;

  if (intensity < 25.0f) return 1500;
  else if (intensity < 40.0f) return 1100;
  else if (intensity < 60.0f) return 700;
  else if (intensity < 80.0f) return 450;
  else return 250;

  return 0;
}