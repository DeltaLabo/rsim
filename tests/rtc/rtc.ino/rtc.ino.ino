#include "Wire.h"
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>

#define DS3231_I2C_ADDRESS 0x68

#define ntpServer "north-america.pool.ntp.org"
#define gmtOffset_sec -6 * 3600 // GMT-6
#define daylightOffset_sec 0 // No daylight savings

#define SDA_PIN 5
#define SCL_PIN 15

#define RX_PIN 44
#define TX_PIN 43

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

HardwareSerial HwSerial(0);

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(
  byte *second,
  byte *minute,
  byte *hour,
  byte *dayOfWeek,
  byte *dayOfMonth,
  byte *month,
  byte *year
)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void readDS3231seconds(byte *second) {
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
  // request 1 byte of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
}

struct tm_bytes {
  byte tm_sec;
  byte tm_min;
  byte tm_hour;
  byte tm_wday;
  byte tm_mday;
  byte tm_mon;
  byte tm_year;
};

bool getLocalTimeinBytes(struct tm_bytes *timeInfo_bytes) {
  struct tm timeInfo;

  bool result = getLocalTime(&timeInfo);

  timeInfo_bytes->tm_sec = byte(timeInfo.tm_sec);
  timeInfo_bytes->tm_min = byte(timeInfo.tm_min);
  timeInfo_bytes->tm_hour = byte(timeInfo.tm_hour);
  timeInfo_bytes->tm_wday = byte(timeInfo.tm_wday);
  timeInfo_bytes->tm_mday = byte(timeInfo.tm_mday);
  timeInfo_bytes->tm_mon = byte(timeInfo.tm_mon + 1); // Months start at 0
  timeInfo_bytes->tm_year = byte(timeInfo.tm_year - 100); // timeInfo only contains offset from 1900

  return result;
}

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  HwSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  HwSerial.printf("%d/%d/%d (%d) - %d:%d:%d\n", dayOfMonth, month, year, dayOfWeek, hour, minute, second);
  delay(500);
}
