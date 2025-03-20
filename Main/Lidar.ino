#include <Wire.h>

#define LIDAR_I2C_ADDR      0x40
#define REG_DISTANCE_UPPER  0x5E
#define REG_DISTANCE_LOWER  0x5F
#define REG_SHIFT           0x35

#define SAMPLE_COUNT        10
#define WATER_THRESHOLD     0.5

float read_register(uint8_t reg) {
    Wire.beginTransmission(LIDAR_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);  // Send repeated start condition
    Wire.requestFrom(LIDAR_I2C_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    Serial.println("Error reading I2C register.");
    return -1;
}

float read_distance() {
    uint8_t dist_upper = read_register(REG_DISTANCE_UPPER);
    uint8_t dist_lower = read_register(REG_DISTANCE_LOWER);
    uint8_t shift = read_register(REG_SHIFT);

    if (dist_upper == -1 || dist_lower == -1 || shift == -1) {
        Serial.println("Error reading distance.");
        return -1;
    }

    // Combine 12-bit raw distance value
    uint16_t distance_raw = ((uint16_t)dist_upper << 4) | (dist_lower & 0x0F);
    
    // Convert to cm: distance_cm = Distance[11:0] / (16 * (2^n))
    float distance_cm = (float)distance_raw / 16.0 / (1 << shift);

    Serial.print("DISTANCE: ");
    Serial.print(distance_cm);
    Serial.println(" cm");

    delay(100);
    return distance_cm;
}

bool detect_water() {
    float distances[SAMPLE_COUNT];
    float sum = 0, mean, variance = 0, stddev;

    // Collect multiple distance readings
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        distances[i] = read_distance();
        sum += distances[i];
        delay(10);  // Small delay between readings
    }

    mean = sum / SAMPLE_COUNT;

    // Compute variance
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        variance += pow(distances[i] - mean, 2);
    }
    variance /= SAMPLE_COUNT;
    stddev = sqrt(variance);

    Serial.print("STD DEV: ");
    Serial.println(stddev);

    return (stddev > WATER_THRESHOLD);
}

void lidarTask(void *pvParameters) {
  Wire.begin();
  while(true){
    if (detect_water()) {
        Serial.println("Water detected on windshield!");
        lidar_rain_detected.store(1, std::memory_order_relaxed);
    } else {
        Serial.println("Windshield is dry.");
        lidar_rain_detected.store(0, std::memory_order_relaxed);
    }
     delay(2000);
  }
}