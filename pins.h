#ifndef PINS_H
#include "slm_params.h"

#define PINS_H

#ifdef XIAO
#define I2S_WS  D2
#define I2S_SCK D3
#define I2S_SD  D8
#else
#ifdef YD
#define I2S_WS  13
#define I2S_SCK 12
#define I2S_SD  11
#endif
#endif

#ifdef XIAO
#define RED_LED_PIN D0 
#define GREEN_LED_PIN D1
#ifdef USE_BLUE_LED
#define BLUE_LED_PIN D9
#endif
#else
#ifdef YD
#define RED_LED_PIN 21
#define GREEN_LED_PIN 35
#ifdef USE_BLUE_LED
#define BLUE_LED_PIN 20
#endif
#endif
#endif

#ifdef XIAO
#define RX_PIN D7
#define TX_PIN D6
#else
#ifdef YD
#define RX_PIN 44
#define TX_PIN 43
#endif
#endif

#endif