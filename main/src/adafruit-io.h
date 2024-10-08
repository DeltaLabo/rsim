#ifndef ADAFRUIT_IO_H
#define ADAFRUIT_IO_H

#include "WiFi.h"
#include "HTTPClient.h"
#include <String.h>

#define MAX_WIFI_RECONNECTION_ATTEMPTS 3

#define ssid = "LaboratorioDelta"
#define password = "labdelta21!"
#define io_key = "aio_OAqO45kIs4r2yFebfjZJ7hhQHD2r"
#define username = "delta_lab"

#define eq_feed_key = "equivalent-noise-level"
#define max_feed_key = "maximum-noise-level"
#define min_feed_key = "minimum-noise-level"

#define WIFI_CHECK_PERIOD 30000 // ms

void wifi_checker_task(void* parameter);
void logToAdafruitIO(const char* value_str, const char* feed_key);

#endif // ADAFRUIT_IO_H