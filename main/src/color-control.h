#ifndef COLOR_CONTROL_H
#define COLOR_CONTROL_H

#include "slm-params.h"
#include "pins.h"
#include "driver/ledc.h"
#include <Arduino.h>

int leqToColor(float Leq_dB);
void resetArray(int* array, int arraySize, int value);
void appendToArray(int* array, int arraySize, int newValue);
int updateColorArray(int currentColor);
void setLEDColor(int color);

extern int colorArray[];
extern bool initColorArray;

#endif // COLOR_CONTROL_H
