// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef SLM_PARAMS_H
#define SLM_PARAMS_H

// Periods
#define LOGGING_PERIOD 16.0 // second(s)
#define RTC_UPDATE_PERIOD 3600000 // millisecond(s), 1 hour
#define ESP32_RESTART_PERIOD 86400000 // milliseconds(s), 24 hours

#define MAX_LATENCY 200 // millisecond(s)
#define SYNC_FAIL_LATENCY 2100 // millisecond(s)
#define MAX_SYNC_FAILURES 200

#define ESPNOW_MAX_INIT_FAILURES 10

// Uncomment to make the RSIM unit synchronize other client units
#define ESPNOW_SERVER

// Comment the above define to let the RSIM unit be synchronized by the server unit
#ifndef ESPNOW_SERVER
#define ESPNOW_CLIENT
#endif

// WiFi parameters
#define WIFI_SSID "LaboratorioDelta"
#define WIFI_PASSWORD "labdelta21!"

// ThingSpeak parameters
#define WRITE_API_KEY "CLXSWMD66IFK7PO4"
#define CHANNEL_NUMBER 2363548

// ESPNOW MAC addresses
const uint8_t broadcastAddress[] = {0x48, 0x27, 0xE2, 0xE6, 0xDC, 0x84}; // For YD

// Indicator color definitions
#define RED 2
#define YELLOW 1
#define GREEN 0

// ledc channels
#define RED_LED_CHANNEL 0
#define GREEN_LED_CHANNEL 1
#define BLUE_LED_CHANNEL 2

#define LEDC_FREQ 5000 // Hz
#define LEDC_RESOLUTION 8 // bits

#define NORMAL 0
#define FREEZE 1
#define SYNCING 2

//
// Configuration
//

// Comment to disable functionality
//#define USE_THINGSPEAK
//#define USE_ESPNOW

#define LEQ_PERIOD        2.0      // second(s)
#define WEIGHTING         A_weighting // 'A_weighting' 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS         "LAeq"      // customize based on above weighting used
#define DB_UNITS          "dBA"       // customize based on above weighting used
#define MIC_OFFSET_DB     1.3         // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

#define GREEN_UPPER_LIMIT  55.0 // dBA, maximum noise level for which the indicator stays green
#define YELLOW_UPPER_LIMIT 70.0 // dBA, maximum noise level for which the indicator stays yellow

#endif