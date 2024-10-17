#include "adafruit-io.h"


extern QueueHandle_t logging_queue;

WiFiClient client;
bool WiFiClientBusy = false;

// State variables to manage request and response
uint8_t pendingResponses = 0;
uint32_t responseTimer = 0;


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
  if (WiFi.status() == WL_CONNECTED) {
    // Construct URL and payload
    String url = String("/api/v2/") + username + "/feeds/" + aio_group + "." + feed_key + "/data";
    String payload = "{\"value\": " + value_str + "}";

    if (!client.connect("io.adafruit.com", 80)) { // port 80 for HTTP
      Serial.println("[ERROR] [LOGGING]: Couldn't connect to Adafruit IO.");
      return;
    }

    // Send the HTTP POST request
    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: io.adafruit.com");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(payload.length());
    client.print("X-AIO-Key: ");
    client.println(io_key);
    client.println(); // End of headers
    client.println(payload); // Body data

    Serial.println("[INFO] [LOGGING]: Attempting to log to Adafruit IO.");

    // Indicate that there's a pending HTTP response
    WiFiClientBusy = true;
    responseTimer = xTaskGetTickCount();
  } else {
    Serial.println("[ERROR] [LOGGING]: Couldn't connect to WiFi.");
  }
}

void checkForHTTPResponse() {
  if (WiFiClientBusy == true) {
    uint32_t currentTime = xTaskGetTickCount();

    // Timeout handling
    if ((currentTime - responseTimer) > pdMS_TO_TICKS(HTTP_RESPONSE_TIMEOUT)) {
      Serial.println("[ERROR] [LOGGING]: Adafruit IO response timeout.");
      client.stop();
      WiFiClientBusy = false;
      return;
    }

    // If the client has data, process the response
    if (client.available()) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break; // Headers end at a blank line
        }
      }

      String response = client.readString(); // Read the response body
      bool errorOccurred = response.indexOf("error") >= 0;
      if (!errorOccurred) {
        Serial.println("[INFO] [LOGGING]: Successfully logged data point to Adafruit IO.");
      } else {
        Serial.print("[ERROR] [LOGGING]: Failed to log data. Response: ");
        Serial.println(response);
      }

      client.stop();

      WiFiClientBusy = false;
    }
  }
}

void logger_task(void* parameter) {
  LogData logData;
  while (true) {
    if (uxQueueMessagesWaiting(logging_queue) > 0 && !WiFiClientBusy) {
        // There are messages in the queue, proceed to read
        if (xQueueReceive(logging_queue, &logData, portMAX_DELAY)) {
          logToAdafruitIO(logData.value, logData.feedKey);
        }
    } else {
      checkForHTTPResponse();
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}