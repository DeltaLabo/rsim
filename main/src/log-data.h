#ifndef LOG_DATA_H
#define LOG_DATA_H

#include <Arduino.h>

// Container class for logged values, with their Adafruit IO key
struct LogData {
  String value;
  const char* feedKey;
};

#endif // LOG_DATA_H
