 #include <WiFi.h>
 
const char* ssid = "wars";
const char* password = "12345678";

//const char* server = "192.168.2.173";  // DAVID
//const char* server = "172.17.129.132"; //McMaster on David's laptop
//const char* server = "192.168.2.27";   // VIKTOR
const char* server = "192.168.1.17"; //Jack's house
const int port = 5000;

WiFiClient client;

std::atomic<bool> wifi_reconnect_in_progress(false);

void wifiReconnectTask(void *pvParameters) {
  Serial.println("WiFi reconnect task started...");

  WiFi.disconnect(true);  // Optional hard reset
  delay(1000);

  wifiSetup();  // Your reconnect logic

  Serial.println("WiFi reconnect task finished.");
  wifi_reconnect_in_progress.store(false);
  vTaskDelete(NULL);  // Kill this task
}

 void wifiSetup()
 {
    // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 7000;  // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed (timeout).");
    wifi_connected.store(0, std::memory_order_relaxed);
  } else {
    Serial.println("\n WiFi connected!");
    wifi_connected.store(1, std::memory_order_relaxed);
  } 
 }

void sendPhotoOverWifi()
{
  if (WiFi.status() != WL_CONNECTED && !wifi_reconnect_in_progress.load()) {
      Serial.println("\nWiFi connection failed (timeout).");
      wifi_connected.store(0, std::memory_order_relaxed);
        Serial.println("WiFi disconnected — starting reconnect task.");
        wifi_reconnect_in_progress.store(true);

        xTaskCreatePinnedToCore(
            wifiReconnectTask,       // Task function
            "WiFiReconnect",         // Name
            4096,                    // Stack size
            NULL,                    // Params
            1,                       // Priority
            NULL,                    // Handle (not needed)
            1                        // Core (can be 0 or 1)
        );
        camera_rain_intensity.store(0, std::memory_order_relaxed);
        return;
    }
  // Get image length from FIFO
  uint32_t length = myCAM.read_fifo_length();
  Serial.print("Read image length: ");
  Serial.println(length);
  if (length == 0 || length > 0x7FFFF) {
    Serial.println(F(" Invalid image length."));
    return;
  }

  Serial.print(F("📦 Image size: "));
  Serial.println(length);

  // Connect to server
  WiFiClient client;
  if (client.connect(server, port)) {  // Replace with your server's IP
    Serial.println(F("Connected to server. Sending image..."));
    client_connected.store(1, std::memory_order_relaxed);

    // Send HTTP POST headers
    client.println("POST /upload HTTP/1.1");
    client.println("Host: 192.168.1.5");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(length);
    client.println();  // End of headers

    myCAM.CS_LOW();
    myCAM.set_fifo_burst();

    const int BURST_SIZE = 256;
    uint8_t buf[BURST_SIZE];
    uint32_t bytes_remaining = length;

    while (bytes_remaining > 0) {
      uint16_t chunk = (bytes_remaining >= BURST_SIZE) ? BURST_SIZE : bytes_remaining;

      for (uint16_t i = 0; i < chunk; i++) {
        buf[i] = SPI.transfer(0x00);  // 🚨 Use SPI.transfer directly
      }

      client.write(buf, chunk);
      bytes_remaining -= chunk;

      if ((bytes_remaining % 4096) < BURST_SIZE) {
        delay(1);  // Prevent watchdog
      }
    }

  myCAM.CS_HIGH();

  Serial.println(F("Image sent! Waiting for response..."));

  String line;
  bool headersEnded = false;
  String body = "";

  while (client.connected()) {
    while (client.available()) {
      char c = client.read();
      line += c;

      // Look for end of headers
      if (!headersEnded && line.endsWith("\r\n\r\n")) {
        headersEnded = true;
        line = "";  // Clear for reading body
        continue;
      }

      // Once headers are done, read body
      if (headersEnded) {
        body += c;
      }
    }
  }

  // Convert and store both rain and fog
  Serial.println("Server message:");
  Serial.println(body);

  float rain_intensity = 0;
  float fog_value = 0;

  // Simple parsing
  int rainIdx = body.indexOf("Rain:");
  int fogIdx = body.indexOf("Fog:");

  if (rainIdx != -1 && fogIdx != -1) {
    String rainStr = body.substring(rainIdx + 5, body.indexOf(",", rainIdx));
    String fogStr = body.substring(fogIdx + 4);

    rainStr.trim();
    fogStr.trim();

    rain_intensity = rainStr.toFloat();

    // You can parse fog as float or check for boolean
    if (fogStr == "True") {
      fog_value = 1;
    } else if (fogStr == "False") {
      fog_value = 0;
    } else {
      fog_value = fogStr.toFloat();  // For numeric fog intensity
    }

    Serial.print("Parsed rain intensity: ");
    Serial.println(rain_intensity);
    Serial.print("Parsed fog: ");
    Serial.println(fog_value);

    camera_rain_intensity.store(rain_intensity, std::memory_order_relaxed);
    camera_fog.store(fog_value, std::memory_order_relaxed);  // NEW: fog variable
  }

  client.stop();

  } else {
    Serial.println(F("Failed to connect to server."));
    client_connected.store(0, std::memory_order_relaxed);

  }
}