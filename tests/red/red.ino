#include <Arduino.h>
#include "driver/ledc.h"

#define RED_LED_PIN 15
#define GREEN_LED_PIN 16
#define BLUE_LED_PIN 17

#define LEDC_FREQ 5000 // Hz
#define LEDC_RESOLUTION 8 // bits

void setup() {
  ledcAttach(RED_LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttach(GREEN_LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttach(BLUE_LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);

  analogWrite(GREEN_LED_PIN, 255);
  analogWrite(BLUE_LED_PIN, 255);
  analogWrite(RED_LED_PIN, 0);
}

void loop() {
  
}