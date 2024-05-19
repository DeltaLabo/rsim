#include "rtc.h"

const char* ntpServer = "north-america.pool.ntp.org";
const long  gmtOffset_sec = -6 * 3600; // GMT-6
const int   daylightOffset_sec = 0; // No daylight savings

struct tm_bytes {
  byte tm_sec;
  byte tm_min;
  byte tm_hour;
  byte tm_wday;
  byte tm_mday;
  byte tm_mon;
  byte tm_year;
} tm_bytes;

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

void Update_RTC(TimerHandle_t xTimer) {
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    HwSerial.print("[INFO] [RTC]: Attempting to connect to WIFI...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (WiFi.status() == WL_CONNECTED){
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm_bytes timeInfo;

    if(getLocalTimeinBytes(&timeInfo)){
      setDS3231time(
        timeInfo.tm_sec,
        timeInfo.tm_min,
        timeInfo.tm_hour,
        timeInfo.tm_wday,
        timeInfo.tm_mday,
        timeInfo.tm_mon,
        timeInfo.tm_year
      );
      HwSerial.println("[INFO] [RTC]: RTC time updated.");
    }
    else HwSerial.println("[ERROR] [RTC]: Could not update RTC time.");
  }
  else HwSerial.println("[ERROR] [RTC]: Could not connect to WiFi.");
}

void awaitEvenSecond() {
  HwSerial.println("[INFO] [RTC]: Awaiting even second...");
  byte s0, s1;
  // Initial time measurement
  readDS3231seconds(&s0);

  // Wait until next second change
  do {
    readDS3231seconds(&s1);
  }
  while(s0 == s1);
  
  // Wait until next even second
  do {
    readDS3231seconds(&s1);
  }
  while(s1 % 2 != 0);

  HwSerial.println("[INFO] [RTC]: Ready to start.")
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