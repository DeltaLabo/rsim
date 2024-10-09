#include "battery-checker.h"

// Battery voltage and current meter
Adafruit_INA219 ina219;

short batteryState = ENOUGH_BATTERY;

void battery_checker_task(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;

  Wire.begin(INA_SDA, INA_SCL); // SDA, SCL

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (!ina219.begin()) {
    Serial.println("[ERROR] [POWER]: Failed to find INA219 chip.");
  } else {
    pinMode(CHARGER_LED_PIN, OUTPUT);
    // Turn off charging indicator LED
    digitalWrite(CHARGER_LED_PIN, HIGH);

  }

  while (true) {
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    current_mA = ina219.getCurrent_mA();

    if (current_mA > MIN_CHARGING_CURRENT) {
      batteryState = CHARGING;
      // Turn on charging indicator LED
      digitalWrite(CHARGER_LED_PIN, LOW);
      Serial.println("[INFO] [POWER]: The battery is charging.");
    } else if (loadvoltage > MIN_CHARGED_VOLTAGE) {
      batteryState = ENOUGH_BATTERY;
      // Turn off charging indicator LED
      digitalWrite(CHARGER_LED_PIN, HIGH);
      Serial.println("[INFO] [POWER]: The battery is charged.");
    } else{
      batteryState = LOW_BATTERY;
      // Turn on charging indicator LED
      digitalWrite(CHARGER_LED_PIN, LOW);
      Serial.println("[WARNING] [POWER]: The battery voltage is low.");
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(BATTERY_CHECK_PERIOD));
  }
}