#include "adafruit-io.h"

void wifi_checker_task(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[INFO] [WIFI]: Not connected, attempting to connect.");
            WiFi.begin(ssid, password);

            // Attempt to connect with limited retries
            for (int i = 0; i < MAX_WIFI_RECONNECTION_ATTEMPTS && WiFi.status() != WL_CONNECTED; i++) {
                vTaskDelay(pdMS_TO_TICKS(5000)); // Delay between reconnection attempts
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("[INFO] [WIFI]: Connected to WiFi.");
            } else {
                Serial.println("[ERROR] [WIFI]: Couldn't connect to WiFi.");
            }
        }

        // Wait for the next check period
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(WIFI_CHECK_PERIOD));
    }
}

void logToAdafruitIO(String value_str, const char* feed_key) {
  // Send HTTP POST request
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("https://io.adafruit.com/api/v2/") + username + "/feeds/" + aio_group + "." + feed_key + "/data";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-AIO-Key", io_key);

    // Data to send with POST request
    String httpRequestData = String("{\"value\": ") + value_str + String("}");

    // Send POST request
    int httpResponseCode = http.POST(httpRequestData);

    // Check response
    if (httpResponseCode > 0) {
      String response = http.getString();

      bool errorOcurred = response.indexOf("error") >= 0;
      if (!errorOcurred) {
        Serial.print("[INFO] [LOGGING]: Successfully logged data point to Adafruit IO feed ");
        Serial.print(feed_key);
        Serial.print(". ");
      } else {
        Serial.print("[ERROR] [LOGGING]: Couldn't log data point to Adafruit IO feed ");
        Serial.print(feed_key);
        Serial.print(", HTTP code: ");
        Serial.print(httpResponseCode);
        Serial.println(".");
      }
    } else {
      Serial.print("[ERROR] [LOGGING]: Couldn't log data point to Adafruit IO feed ");
      Serial.print(feed_key);
      Serial.print(", HTTP code: ");
      Serial.print(httpResponseCode);
      Serial.println(".");
    }

    // End HTTP connection
    http.end();
  } else {
    Serial.println("[ERROR] [LOGGING]: Couldn't connect to WiFi.");
  }
}