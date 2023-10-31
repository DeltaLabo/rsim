#include "LEDStripDriver.h"

//Conexiones al driver:
// DIN=GPIO6 CIN=GPIO7 
LEDStripDriver led = LEDStripDriver(6, 7);

void setup() {
  //Código de colores RGB:
  led.setColor(0, 255, 0); // Verde
  led.setColor(255, 255, 0); // Amarillo
  led.setColor(255, 0, 0); // Rojo 
  led.setColor(0, 0, 0); // Apagado
}
void loop() {}