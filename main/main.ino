#include <Arduino.h>
#include "driver/i2s.h"
#include <stdint.h>

#include "src/slm.h"
#include "src/adafruit-io.h"
#include "src/battery-checker.h"

#include "checks.h"


// Set to enable/disable functionality
bool USE_BATTERY = false;
bool USE_LOGGING = true;


QueueHandle_t samples_queue;
QueueHandle_t logging_queue;


// FreeRTOS priority and stack sizes (in 32-bit words) 
#define I2S_TASK_PRI 5
#define I2S_TASK_STACK 4096
#define LEQ_TASK_PRI 5
#define LEQ_TASK_STACK 8192
#define BAT_TASK_PRI 1
#define BAT_TASK_STACK 4096
#define WIFI_TASK_PRI 1
#define WIFI_TASK_STACK 4096
#define LOGGER_TASK_PRI 3
#define LOGGER_TASK_STACK 8192


void setup() {
  setCpuFrequencyMhz(240);

  // Init serial for logging
  Serial.begin(115200);

  // Create FreeRTOS queue
  samples_queue = xQueueCreate(100, sizeof(float));

  if (USE_BATTERY == true) {
    xTaskCreatePinnedToCore(battery_checker_task, "Battery Checker", BAT_TASK_STACK, NULL, BAT_TASK_PRI, NULL, 1);
  } else {
    Serial.println("[INFO] [POWER]: Battery checking disabled.");
  }

  if (USE_LOGGING == true) {
    // Create the WiFi connection task and pin it to the second core (ID=1)
    xTaskCreatePinnedToCore(wifi_checker_task, "WiFi Checker", WIFI_TASK_STACK, NULL, WIFI_TASK_PRI, NULL, 1);
  
    // Initialize the logging queue
    logging_queue = xQueueCreate(LOGGING_QUEUE_SIZE, sizeof(LogData));

    // Create the logger task and pin it to the second core (ID=1)
    xTaskCreatePinnedToCore(logger_task, "Logger", LOGGER_TASK_STACK, NULL, LOGGER_TASK_PRI, NULL, 1);
  } else {
    Serial.println("[INFO] [LOGGING]: Logging disabled.");
  }
  // Create the mic reader task and pin it to the first core (ID=0)
  xTaskCreatePinnedToCore(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL, 0);

  // Create the Leq calculator task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(leq_calculator_task, "Leq Calculator", LEQ_TASK_STACK, NULL, LEQ_TASK_PRI, NULL, 1);


  vTaskDelay(pdMS_TO_TICKS(1000)); // Safety
}

void loop() {
  // Execution will never reach this point since we're using FreeRTOS
}