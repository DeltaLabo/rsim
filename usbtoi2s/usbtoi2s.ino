#include "Audio/Audio.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputUSB            usb1;           //xy=119.42855834960938,189.4285659790039
AudioOutputI2Sslave      i2sslave1;      //xy=283.42866134643555,189.42855834960938
AudioConnection          patchCord1(usb1, 0, i2sslave1, 0);
AudioConnection          patchCord2(usb1, 1, i2sslave1, 1);
// GUItool: end automatically generated code

