#include "adafruit-io.h"


extern QueueHandle_t logging_queue;

HTTPClient http;

void wifi_checker_task(void* parameter) {
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[INFO] [WIFI]: Not connected, attempting to connect.");

      int attemptCounter = 0;
      do {
        WiFi.begin(ssid, password);
        attemptCounter++;
        vTaskDelay(pdMS_TO_TICKS(5000));
      } while (WiFi.status() != WL_CONNECTED && attemptCounter<MAX_WIFI_RECONNECTION_ATTEMPTS);

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[INFO] [WIFI]: Connected to WiFi.");
      } else {
        Serial.println("[ERROR] [WIFI]: Couldn't connect to WiFi. Will retry later.");
      }
    }
    // Wait for the next check period
    vTaskDelay(pdMS_TO_TICKS(WIFI_CHECK_PERIOD));
  }
}

void logToAdafruitIO(const String &value_str, const String &feed_key) {
  // Send HTTP POST request
  if (WiFi.status() == WL_CONNECTED) {
    String url = String("https://io.adafruit.com/api/v2/") + username + "/feeds/" + aio_group + "." + feed_key + "/data";
    http.begin(url);
    http.setConnectTimeout(3000); // ms
    http.setTimeout(1); // s
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
        Serial.println(". ");
      } else {
        Serial.print("[ERROR] [LOGGING]: Couldn't log data point to Adafruit IO feed ");
        Serial.print(feed_key);
        Serial.print(", Error: ");
        Serial.print(response);
        Serial.println(".");
      }
    } else {
      Serial.print("[ERROR] [LOGGING]: Couldn't log data point to Adafruit IO feed ");
      Serial.print(feed_key);
      Serial.print(", HTTP code: ");
      Serial.print(httpResponseCode);
      if (httpResponseCode == -1) {Serial.print(" (connection refused)");}
      else if (httpResponseCode == -4) {Serial.print(" (not connected)");}
      else if (httpResponseCode == -5) {Serial.print(" (connection lost)");}
      else if (httpResponseCode == -11) {Serial.print(" (timeout)");}
      Serial.println(".");
    }

    // End HTTP connection
    http.end();
  } else {
    Serial.println("[ERROR] [LOGGING]: Couldn't connect to WiFi.");
  }
}

void logger_task(void* parameter) {
  LogData logData;
  while (true) {
    if (xQueueReceive(logging_queue, &logData, portMAX_DELAY)) {
      logToAdafruitIO(logData.value, logData.feedKey);
    }
  }
}