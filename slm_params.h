// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef PARAMS_H
#define PARAMS_H

// ESP models
#define YD // XIAO or YD

// Periods
#define LOGGING_PERIOD 64.0 // second(s)
#define RTC_UPDATE_PERIOD 3600000 // millisecond(s), 1 hour
#define ESP32_RESTART_PERIOD 86400000 // milliseconds(s), 24 hours

#define MAX_LATENCY 200 // millisecond(s)
#define SYNC_FAIL_LATENCY 2100 // millisecond(s)
#define MAX_SYNC_FAILURES 200

// WiFi parameters
#define WIFI_SSID "LaboratorioDelta"
#define WIFI_PASSWORD "labdelta21!"

// ThingSpeak parameters
#define WRITE_API_KEY "CLXSWMD66IFK7PO4"
#define CHANNEL_NUMBER 2363548

#ifdef XIAO
// ESPNOW MAC addresses
const uint8_t broadcastAddress[] = {0xC0, 0x4E, 0x30, 0x3A, 0x03, 0x34}; // For Xiao
#else
#ifdef YD
const uint8_t broadcastAddress[] = {0x48, 0x27, 0xE2, 0xE6, 0xDC, 0x84}; // For YD
#endif
#endif

// Indicator color definitions
#define RED 2
#define YELLOW 1
#define GREEN 0

// Comment to leave BLUE_LED_PIN unused
//#define USE_BLUE_LED

//
// Configuration
//

// Select how to log measurements
// WiFi means ThingSpeak logging
#define WIFI 0
#define SERIAL 1
#define WIFI_PLUS_SERIAL 2
#define LOG_MODE SERIAL

#define LEQ_PERIOD        2.0      // second(s)
#define WEIGHTING         A_weighting // 'A_weighting' 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS         "LAeq"      // customize based on above weighting used
#define DB_UNITS          "dBA"       // customize based on above weighting used
#define MIC_OFFSET_DB     1.3         // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

#define GREEN_UPPER_LIMIT  55.0 // dBA, maximum noise level for which the indicator stays green
#define YELLOW_UPPER_LIMIT 70.0 // dBA, maximum noise level for which the indicator stays yellow

#endif