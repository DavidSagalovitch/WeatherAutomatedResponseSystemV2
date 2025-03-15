#include <Arduino.h>
#include <math.h>

#define WIDTH  640   // Image width
#define HEIGHT 480   // Image height
#define EDGE_THRESHOLD 50  // Edge detection threshold
#define BLOB_MIN_SIZE 15   // Minimum blob size to be considered a raindrop

// Rolling buffer for 3-row edge detection
uint8_t row_buffer[3][WIDTH];  
uint8_t edge_buffer[WIDTH];  // Edge tracking buffer
int brightness_sum = 0;
int edge_count = 0;
int blob_count = 0;

// Function Prototypes
void process_streamed_image();
void detect_edges_stream(uint8_t new_row[], uint8_t edge_row[]);
void detect_blobs_stream(uint8_t edge_row[]);
float estimate_rain_intensity(int edge_count, int blob_count, int brightness_avg);

void process_streamed_image() {
    brightness_sum = 0;
    edge_count = 0;
    blob_count = 0;

    Serial.println("IMAGE_PROCESS_START");

    // Initialize buffers
    memset(row_buffer, 0, sizeof(row_buffer));
    memset(edge_buffer, 0, sizeof(edge_buffer));

    // **Processing Image in 240-row Blocks to Reduce RAM Usage**
    const int row_block_size = 240;  // Process 240 rows at a time
    const int tile_width = 320;      // Keep tile width at 320px
    const int BURST_SIZE = 256;      // Burst read buffer size
    uint8_t burst_buffer[BURST_SIZE];  // Temporary buffer for burst reads

    for (int tile_y = 0; tile_y < HEIGHT; tile_y += row_block_size) {
        for (int tile_x = 0; tile_x < WIDTH; tile_x += tile_width) {
            Serial.print("Processing Tile: ");
            Serial.print(tile_x);
            Serial.print(",");
            Serial.println(tile_y);

            // Process 240 rows at a time
            for (int y = 0; y < row_block_size; y++) {
                if (tile_y + y >= HEIGHT) break; // Prevent overflow

                int bytes_to_read = tile_width;  // Number of bytes per row
                int buffer_index = 0;  // Track position in row_buffer

                while (bytes_to_read > 0) {
                    int chunk_size = min(BURST_SIZE, bytes_to_read);  // Read in chunks

                    // **Manual Burst Read**
                    for (int i = 0; i < chunk_size; i++) {
                        row_buffer[2][buffer_index + i] = myCAM.read_fifo();  // Read directly into row_buffer
                    }

                    // Accumulate brightness
                    for (int i = 0; i < chunk_size; i++) {
                        brightness_sum += row_buffer[2][buffer_index + i]; 
                    }

                    buffer_index += chunk_size;
                    bytes_to_read -= chunk_size;
                }

                // Perform edge detection on the middle row
                detect_edges_stream(row_buffer[1], edge_buffer);

                // Detect blobs from edge map
                detect_blobs_stream(edge_buffer);

                // Shift row buffers (FIFO-style rolling buffer)
                memcpy(row_buffer[0], row_buffer[1], tile_width);
                memcpy(row_buffer[1], row_buffer[2], tile_width);
                
            }
             vTaskDelay(1);
        }
             
    }
    myCAM.flush_fifo();
    myCAM.clear_fifo_flag();

    // Compute final results
    int brightness_avg = brightness_sum /(float)(WIDTH * HEIGHT);
    float rain_intensity = estimate_rain_intensity(edge_count, blob_count, brightness_avg);

    Serial.print("Brightness: ");
    Serial.print(brightness_avg);
    Serial.print(", Edges: ");
    Serial.print(edge_count);
    Serial.print(", Blobs: ");
    Serial.print(blob_count);
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

// **Blob Detection using 1D Edge Tracking**
void detect_blobs_stream(uint8_t edge_row[]) {
    static int in_blob = 0;
    static int blob_size = 0;

    for (int x = 0; x < WIDTH; x++) {
        if (edge_row[x] == 255) {
            if (!in_blob) {
                in_blob = 1;
                blob_size = 1;
            } else {
                blob_size++;
            }
        } else {
            if (in_blob && blob_size >= BLOB_MIN_SIZE) {
                blob_count++;  // Count blob if large enough
            }
            in_blob = 0;
            blob_size = 0;
        }
    }
}

// **Estimate rain intensity based on edge & blob counts**
float estimate_rain_intensity(int edge_count, int blob_count, int brightness_avg) {
    float blob_factor = logf(1 + blob_count) / logf(1000.0);
    float brightness_weight = brightness_avg / 255.0;  
    float intensity = (brightness_weight * blob_factor * 50);

    if (intensity > 100) intensity = 100;
    if (intensity < 0) intensity = 0;
    return intensity;
}