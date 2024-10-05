// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef SLM_PARAMS_H
#define SLM_PARAMS_H

// Periods
#define LOGGING_PERIOD 16.0 // second(s)
#define RTC_UPDATE_PERIOD 3600000 // millisecond(s), 1 hour
#define ESP32_RESTART_PERIOD 86400000 // millisecond(s), 24 hours
#define BATTERY_CHECK_PERIOD 60000 // millisecond(s), 1 minute

// Indicator color definitions
#define RED 2
#define YELLOW 1
#define GREEN 0

// Amount of measurements to calculate the average color over
#define COLOR_WINDOW_SIZE 5

#define LEDC_FREQ 5000 // Hz
#define LEDC_RESOLUTION 8 // bits

// Power states
#define LOW_BATTERY 0
#define ENOUGH_BATTERY 1
#define CHARGING 2

// Power constants
#define MIN_CHARGING_CURRENT 200 // mA
#define MIN_CHARGED_VOLTAGE 12.8 // V

// Comment to disable
//#define USE_BATTERY

//
// SLM Configuration
//

#define LEQ_PERIOD        2.0      // second(s)
#define MIC_OFFSET_DB     1.3         // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

#define GREEN_UPPER_LIMIT  57.0 // dBA, maximum noise level for which the indicator stays green
#define YELLOW_UPPER_LIMIT 70.0 // dBA, maximum noise level for which the indicator stays yellow

#endif