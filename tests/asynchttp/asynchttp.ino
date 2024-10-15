#include <WiFi.h>
#include "secrets.h"

WiFiClient client;
const char* host = "io.adafruit.com";
const int port = 80; // For HTTP. Use WiFiClientSecure if HTTPS is needed

// State variables to manage request and response
bool waitingForResponse = false;
unsigned long requestTimer = 0;
unsigned long logTimer = 0;

// Replace with your credentials
#define ssid "LaboratorioDelta"
// WiFi password and AIO key defined in secrets.h
#define username "delta_lab"
#define aio_group "rsim-v3-max-peralta"

// Adafruit IO feed keys for logging
#define eq_feed_key "equivalent-noise-level"
#define max_feed_key "maximum-noise-level"
#define min_feed_key "minimum-noise-level"

void logToAdafruitIO(const String &value_str, const String &feed_key) {
  if (WiFi.status() == WL_CONNECTED) {
    // Construct URL and payload
    String url = String("/api/v2/") + username + "/feeds/" + aio_group + "." + feed_key + "/data";
    String payload = "{\"value\": " + value_str + "}";

    if (!client.connect(host, port)) {
      Serial.println("[ERROR] [LOGGING]: Connection to Adafruit IO failed.");
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

    // Set the state to indicate we're waiting for a response
    waitingForResponse = true;
    requestTimer = millis(); // Record the time the request was sent
  } else {
    Serial.println("[ERROR] [LOGGING]: Couldn't connect to WiFi.");
  }
}

void checkForResponse() {
  if (waitingForResponse) {
    // Timeout handling
    if (millis() - requestTimer > 5000) { // 5-second timeout
      Serial.println("[ERROR] [LOGGING]: Response timeout.");
      client.stop();
      waitingForResponse = false;
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
        Serial.println(response);
      } else {
        Serial.print("[ERROR] [LOGGING]: Failed to log data. Response: ");
        Serial.println(response);
      }

      client.stop();  // Close the connection
      waitingForResponse = false;  // Reset the state
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi (replace with your own WiFi credentials)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  logTimer = millis();
}

void loop() {
  // Check if a response has been received from the server
  checkForResponse();

  // Your other code can run asynchronously here
  if (millis() - logTimer >= 15000) {
    // Send the initial request (example)
    logToAdafruitIO("25.5", eq_feed_key);
    logTimer = millis();
  }
}
