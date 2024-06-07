#include "rtc.h"

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