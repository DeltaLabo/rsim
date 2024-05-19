#include "Wire.h"
#include <WiFi.h>
#include "time.h"
#include "slm_params.h"
#include <HardwareSerial.h>

#define DS3231_I2C_ADDRESS 0x68

byte decToBcd(byte val);
byte bcdToDec(byte val);

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year);

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

void Update_RTC(TimerHandle_t xTimer);

void awaitEvenSecond();

bool getLocalTimeinBytes(struct tm_bytes *timeInfo_bytes);