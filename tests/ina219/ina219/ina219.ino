#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

// charging = I > 200 mA : turn on red LED
// enough_battery = I <= 200 mA, V > 12.8 V : nothing
// low_battery = I <= 200 mA, V <= 12.8 V : blink blue

void setup(void) 
{
  Serial.begin(115200);
  Wire.begin(D5, D6); // SDA, SCL
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  Serial.println("Measuring voltage and current with INA219 ...");
}

void loop(void) 
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  if (current_mA > 200) {
    Serial.println("State:         Charging");
  } else if (loadvoltage > 12.8) {
    Serial.println("State:         Charged");
  } else{
    Serial.println("State:          Low battery");
  }
  Serial.println("");

  delay(2000);
}
