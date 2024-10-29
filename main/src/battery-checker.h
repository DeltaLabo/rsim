#ifndef BATTERY_CHECKER_H
#define BATTERY_CHECKER_H

#include <Adafruit_INA219.h>
#include <Wire.h>

#include "pins.h"
#include "adafruit-io.h"

// Power constants
#define MIN_CONNECTED_CURRENT 10 // mA
#define MIN_CHARGING_CURRENT 200 // mA
#define MIN_CHARGED_VOLTAGE 12.8 // V

// Power states
#define LOW_BATTERY 0
#define ENOUGH_BATTERY 1
#define CHARGING 2
#define CHARGED 3

#define BATTERY_CHECK_PERIOD 60000 // ms, 1 minute


void battery_checker_task(void* parameter);

#endif // BATTERY_CHECKER_H