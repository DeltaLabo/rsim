#ifndef ADAFRUIT_IO_H
#define ADAFRUIT_IO_H

#include <Arduino.h>
#include "WiFi.h"

#include "secrets.h"
#include "log-data.h"


#define MAX_WIFI_RECONNECTION_ATTEMPTS 3

#define ssid "LaboratorioDelta"
// WiFi password and AIO key defined in secrets.h
#define username "delta_lab"
#define aio_group "rsim-v3-max-peralta"

#define WIFI_CHECK_PERIOD 30000 // ms

#define HTTP_RESPONSE_TIMEOUT 5000 // ms

// Adafruit IO feed keys for logging
#define eq_feed_key "equivalent-noise-level"
#define max_feed_key "maximum-noise-level"
#define min_feed_key "minimum-noise-level"
#define bat_feed_key "battery-state"


void wifi_checker_task(void* parameter);
void logToAdafruitIO(const String &value_str, const char* feed_key);
void logger_task(void* parameter);
void checkForHTTPResponse();

#endif // ADAFRUIT_IO_H