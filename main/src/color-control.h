#ifndef COLOR_CONTROL_H
#define COLOR_CONTROL_H

#include "driver/ledc.h"
#include <Arduino.h>

#include "pins.h"

// Indicator color definitions
#define RED 2
#define YELLOW 1
#define GREEN 0

// Amount of measurements to calculate the average color over
#define COLOR_WINDOW_SIZE 5

#define GREEN_UPPER_LIMIT  57.0 // dBA, maximum noise level for which the indicator stays green
#define YELLOW_UPPER_LIMIT 70.0 // dBA, maximum noise level for which the indicator stays yellow

#define LEDC_FREQ 5000 // Hz
#define LEDC_RESOLUTION 8 // bits

int leqToColor(float Leq_dB);
void resetArray(int* array, int arraySize, int value);
void appendToArray(int* array, int arraySize, int newValue);
int updateColorArray(int currentColor);
void setLEDColor(int color);
void initColorPins();

#endif // COLOR_CONTROL_H
