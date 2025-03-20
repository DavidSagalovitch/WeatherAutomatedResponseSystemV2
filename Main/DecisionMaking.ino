#include <atomic>

std::atomic<float> camera_rain_intensity(0);
std::atomic<uint16_t> camera_rain_detected(0);
std::atomic<bool> is_day(0);
std::atomic<bool> lidar_rain_detected(0);
std::atomic<bool> lidar_rain_intensity(0);

uint16_t getWiperSpeed()
{
  bool day = is_day.load(std::memory_order_relaxed)

  if(day)
  {

  }
  else{
    
  }
  return 0;
}