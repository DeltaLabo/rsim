#ifndef RTC_H
#define RTC_H

#include "Wire.h"
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>

#define DS3231_I2C_ADDRESS 0x68

#define ntpServer "north-america.pool.ntp.org"
#define gmtOffset_sec -6 * 3600 // GMT-6
#define daylightOffset_sec 0 // No daylight savings

byte decToBcd(byte val);
byte bcdToDec(byte val);

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year);

struct tm_bytes {
  byte tm_sec;
  byte tm_min;
  byte tm_hour;
  byte tm_wday;
  byte tm_mday;
  byte tm_mon;
  byte tm_year;
};

void readDS3231time(
    byte *second,
    byte *minute,
    byte *hour,
    byte *dayOfWeek,
    byte *dayOfMonth,
    byte *month,
    byte *year
);

void readDS3231seconds(byte *second);

bool getLocalTimeinBytes(struct tm_bytes *timeInfo_bytes);

#endif