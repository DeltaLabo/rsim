#include "rtc.h"

const char* ntpServer = "north-america.pool.ntp.org";
const long  gmtOffset_sec = -6 * 3600; // GMT-6
const int   daylightOffset_sec = 0; // No daylight savings

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

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
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

void Update_RTC() {
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    if (LOG_MODE == WIFI_PLUS_SERIAL) hwSerial.print("Attempting to connect to WIFI...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  }

  if (WiFi.status() == WL_CONNECTED){
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeInfo;

    if(getLocalTime(&timeinfo)){
      setDS3231time(
        timeInfo.tm_sec,
        timeInfo.tm_min,
        timeInfo.tm_hour,
        timeInfo.tm_wday,
        timeInfo.tm_mday,
        timeInfo.tm_mon + 1, // Months start at 0
        timeInfo.tm_year + 1900 // timeInfo only contains offset from 1900
      ); 
    }
  }
}

void awaitEvenSecond() {
  byte second;

  do {
    readDS3231seconds(second);
  }
  while(second % 2 != 0)
}