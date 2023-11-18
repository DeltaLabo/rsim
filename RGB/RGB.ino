#include "LEDStripDriver.h"

//Conexiones al driver:
// DIN=GPIO6 CIN=GPIO7 
LEDStripDriver led = LEDStripDriver(5, 6);

void setup() {
  Serial.begin(9600);
  led.setColor(0, 0, 0); // Apagado
}
void loop() {
    //Código de colores RGB:
  led.setColor(0, 255, 0); // Verde
  Serial.println("Verde");
  delay(1000);
  led.setColor(255, 255, 0); // Amarillo
  Serial.println("A");
  delay(1000);
  led.setColor(255, 0, 0); // Rojo
  Serial.println("R");
  delay(1000);
}