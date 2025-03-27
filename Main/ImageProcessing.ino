#include <Arduino.h>
#include <math.h>

#define WIDTH  320   // Image width
#define HEIGHT 240   // Image height
#define EDGE_THRESHOLD 50  // Edge detection threshold
#define BLOB_MIN_SIZE 20   // Minimum blob size to be considered a raindrop

// Rolling buffer for 3-row edge detection
uint8_t row_buffer[3][WIDTH];  
uint8_t edge_buffer[WIDTH];  // Edge tracking buffer
int brightness_sum = 0;
int edge_count = 0;
int blob_count = 0;

// Function Prototypes
void process_image();
void detect_edges_stream(uint8_t new_row[], uint8_t edge_row[]);
void detect_blobs_stream(uint8_t edge_row[]);
float estimate_rain_intensity(int edge_count, int blob_count, int brightness_avg);

void process_image() {
    brightness_sum = 0;
    edge_count = 0;
    blob_count = 0;
    int total_blob_area = 0;

    Serial.println("IMAGE_PROCESS_START");

    memset(row_buffer, 0, sizeof(row_buffer));
    memset(edge_buffer, 0, sizeof(edge_buffer));

    const int row_block_size = 240;
    const int tile_width = 320;
    const int BURST_SIZE = 256;
    uint8_t burst_buffer[BURST_SIZE];

    for (int tile_y = 0; tile_y < HEIGHT; tile_y += row_block_size) {
        for (int tile_x = 0; tile_x < WIDTH; tile_x += tile_width) {
            Serial.print("Processing Tile: ");
            Serial.print(tile_x);
            Serial.print(",");
            Serial.println(tile_y);

            for (int y = 0; y < row_block_size; y++) {
                if (tile_y + y >= HEIGHT) break;

                int bytes_to_read = tile_width;
                int buffer_index = 0;

                while (bytes_to_read > 0) {
                    int chunk_size = min(BURST_SIZE, bytes_to_read);

                    for (int i = 0; i < chunk_size; i++) {
                        row_buffer[2][buffer_index + i] = myCAM.read_fifo();
                    }

                    for (int i = 0; i < chunk_size; i++) {
                        brightness_sum += row_buffer[2][buffer_index + i];
                    }

                    buffer_index += chunk_size;
                    bytes_to_read -= chunk_size;
                }

                detect_edges_stream(row_buffer[1], edge_buffer);
                detect_blobs_stream(edge_buffer);  // Updated to count blobs

                memcpy(row_buffer[0], row_buffer[1], tile_width);
                memcpy(row_buffer[1], row_buffer[2], tile_width);
            }

            vTaskDelay(1);
        }
    }

    // Estimate total blob area
    int estimated_blob_area_per_blob = 120;  // midpoint of 80–800 in OpenCV
    total_blob_area = blob_count * estimated_blob_area_per_blob;
    if (edge_count < 100 && blob_count < 10) {
        Serial.println("Flat scene detected — forcing rain intensity to 0");
        camera_rain_intensity.store(0, std::memory_order_relaxed);
        return;
    }
    int brightness_avg = brightness_sum / (WIDTH * HEIGHT);
    float rain_intensity = estimate_rain_intensity(total_blob_area, brightness_avg);

    Serial.print("Brightness: ");
    Serial.print(brightness_avg);
    Serial.print(", Blob Count: ");
    Serial.print(blob_count);
    Serial.print(", Estimated Total Blob Area: ");
    Serial.print(total_blob_area);
    Serial.print(", Estimated Rain Intensity: ");
    Serial.println(rain_intensity, 2);

    Serial.println("IMAGE_PROCESS_END");
}


// **Edge detection using Sobel filter in stream processing**
void detect_edges_stream(uint8_t new_row[], uint8_t edge_row[]) {
    for (int x = 1; x < WIDTH - 1; x++) {
        int gx = -row_buffer[0][x-1] + row_buffer[0][x+1] 
                 -2 * new_row[x-1] + 2 * new_row[x+1] 
                 -row_buffer[2][x-1] + row_buffer[2][x+1];

        int gy = -row_buffer[0][x-1] - 2 * row_buffer[0][x] - row_buffer[0][x+1] 
                 +row_buffer[2][x-1] + 2 * row_buffer[2][x] + row_buffer[2][x+1];

        int magnitude = sqrtf(gx * gx + gy * gy);
        edge_row[x] = (magnitude > EDGE_THRESHOLD) ? 255 : 0;
        if (edge_row[x] > 0) edge_count++;
    }
}

void detect_blobs_stream(uint8_t edge_row[]) {
    static int in_blob = 0;
    static int blob_size = 0;
    static int brightness_sum_blob = 0;

    const int BLOB_MAX_SIZE = 80;    // Approximate OpenCV upper bound
    const int MIN_BRIGHTNESS = 120;   // Match OpenCV brightness filter

    for (int x = 0; x < WIDTH; x++) {
        uint8_t edge = edge_row[x];
        uint8_t brightness = row_buffer[1][x];  // center row

        if (edge == 255) {
            if (!in_blob) {
                in_blob = 1;
                blob_size = 1;
                brightness_sum_blob = brightness;
            } else {
                blob_size++;
                brightness_sum_blob += brightness;
            }

            // If blob gets too big, discard it
            if (blob_size > BLOB_MAX_SIZE) {
                in_blob = 0;
                blob_size = 0;
                brightness_sum_blob = 0;
            }

        } else {
            if (in_blob) {
                float avg_brightness = brightness_sum_blob / (float)blob_size;

                if (blob_size >= BLOB_MIN_SIZE && avg_brightness > MIN_BRIGHTNESS) {
                    blob_count++;  // Valid blob, similar to OpenCV filter
                }

                in_blob = 0;
                blob_size = 0;
                brightness_sum_blob = 0;
            }
        }
    }

    // Handle edge blob at end of row
    if (in_blob) {
        float avg_brightness = brightness_sum_blob / (float)blob_size;
        if (blob_size >= BLOB_MIN_SIZE && avg_brightness > MIN_BRIGHTNESS) {
            blob_count++;
        }
        in_blob = 0;
        blob_size = 0;
        brightness_sum_blob = 0;
    }
}


float estimate_rain_intensity(int total_blob_area, int brightness_avg) {
    float image_area = WIDTH * HEIGHT;
    float coverage_ratio = (float)total_blob_area / image_area;
    float intensity = coverage_ratio * 10000.0f;

    if (intensity > 100) intensity = 100;
    if (intensity < 0) intensity = 0;

    is_day.store((brightness_avg > 50) ? 1 : 0, std::memory_order_relaxed);
    camera_rain_intensity.store(intensity, std::memory_order_relaxed);

    return intensity;
}
