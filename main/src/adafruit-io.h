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


void wifi_checker_task(void* parameter);
void logToAdafruitIO(const String &value_str, const String &feed_key);
void logger_task(void* parameter);
void checkForHTTPResponse();

#endif // ADAFRUIT_IO_H