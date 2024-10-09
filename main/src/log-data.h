#ifndef LOG_DATA_H
#define LOG_DATA_H

#include <Arduino.h>

// Container class for logged values, with their Adafruit IO key
struct LogData {
    String value;
    String feedKey;
};

#endif // LOG_DATA_H
