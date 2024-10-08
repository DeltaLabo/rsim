#ifndef ADAFRUIT_IO_H
#define ADAFRUIT_IO_H

#include "WiFi.h"
#include "HTTPClient.h"
#include <String.h>

#include "secrets.h"

#define MAX_WIFI_RECONNECTION_ATTEMPTS 3

#define ssid "LaboratorioDelta"
// WiFi password and AIO key defined in secrets.h
#define username "delta_lab"

#define aio_group "rsim-v3-max-peralta"

#define eq_feed_key "equivalent-noise-level"
#define max_feed_key "maximum-noise-level"
#define min_feed_key "minimum-noise-level"

#define WIFI_CHECK_PERIOD 30000 // ms

void wifi_checker_task(void* parameter);
void logToAdafruitIO(String value_str, const char* feed_key);

#endif // ADAFRUIT_IO_H