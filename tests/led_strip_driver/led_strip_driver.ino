#include "LEDStripDriver.h"

// use two available GPIO pins from your board
// DIN=GPIO3, CIN=GPIO2 in this example
LEDStripDriver led = LEDStripDriver(3, 2);

void setup() {
  // put your setup code here, to run once:
  led.setColor(210, 100, 0); // RGB
}

void loop() {
  // put your main code here, to run repeatedly
}