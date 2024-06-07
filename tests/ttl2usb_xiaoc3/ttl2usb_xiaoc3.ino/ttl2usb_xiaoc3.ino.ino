#include <HardwareSerial.h>

HardwareSerial HwSerial(1);

String inputString = "";   // A string to hold incoming data
bool stringComplete = false;  // Whether the string is complete

void setup() {
  // Start the HwSerial communication with a baud rate of 9600
  HwSerial.begin(115200, SERIAL_8N1, D7, D6);
  Serial.begin(115200);
}

void loop() {
  // Check if a string is complete (newline character received)
  if (stringComplete) {
    // Print the received string to the HwSerial Monitor
    Serial.print(inputString);
    
    // Clear the string for the next input
    inputString = "";
    stringComplete = false;
  }

  // Read incoming HwSerial data
  while (HwSerial .available() > 0) {
    // Get the new byte:
    char incomingByte = HwSerial.read();
    
    // Add the incoming byte to the string:
    inputString += incomingByte;
    
    // Check if the incoming byte is a newline character:
    if (incomingByte == '\n') {
      stringComplete = true;
    }
  }
}

