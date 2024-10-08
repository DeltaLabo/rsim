#include <Arduino.h>

#include "src/slm.h"
#include "src/color-control.h"
#include "src/adafruit-io.h"
#include "src/battery-checker.h"
#include "src/sos-iir-filter-xtensa.h"


// Comment to disable functionality
//#define USE_BATTERY
#define USE_LOGGING


QueueHandle_t samples_queue;
QueueHandle_t logging_queue;


// DC-Blocker filter - removes DC component from I2S data
// See: https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
// a1 = -0.9992 should heavily attenuate frequencies below 10Hz
SOS_IIR_Filter DC_BLOCKER = {
  1.0, // gain
  {{-1.0, 0.0, +0.9992, 0}} // sos
};

// 
// Equalizer IIR filters to flatten microphone frequency response
// Fs = 48Khz.
//
// Filters are represented as Second-Order Sections cascade with assumption
// that b0 and a0 are equal to 1.0 and 'gain' is applied at the last step 
// B and A coefficients were transformed with GNU Octave: 
// [sos, gain] = tf2sos(B, A)
// See: https://www.dsprelated.com/freebooks/filters/Series_Second_Order_Sections.html
// NOTE: SOS matrix 'a1' and 'a2' coefficients are negatives of tf2sos output
//


// TDK/InvenSense INMP441
// Datasheet: https://www.invensense.com/wp-content/uploads/2015/02/INMP441.pdf
// B ~= [1.00198, -1.99085, 0.98892]
// A ~= [1.0, -1.99518, 0.99518]
SOS_IIR_Filter INMP441 = {
  1.00197834654696, 
  { // Second-Order Sections {b1, b2, -a1, -a2}
    {-1.986920458344451, +0.986963226946616, +1.995178510504166, -0.995184322194091}
  }
};

//
// A-weighting IIR Filter, Fs = 48KHz 
// (By Dr. Matt L., Source: https://dsp.stackexchange.com/a/36122)
// B = [0.169994948147430, 0.280415310498794, -1.120574766348363, 0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003]
// A = [1.0, -2.12979364760736134, 0.42996125885751674, 1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968]
SOS_IIR_Filter A_weighting = {
  0.169994948147430, 
  { // Second-Order Sections {b1, b2, -a1, -a2}
    {-2.00026996133106, +1.00027056142719, -1.060868438509278, -0.163987445885926},
    {+4.35912384203144, +3.09120265783884, +1.208419926363593, -0.273166998428332},
    {-0.70930303489759, -0.29071868393580, +1.982242159753048, -0.982298594928989}
  }
};


// FreeRTOS priority and stack sizes (in 32-bit words) 
#define I2S_TASK_PRI   4
#define I2S_TASK_STACK 4096
#define LEQ_TASK_PRI   3
#define LEQ_TASK_STACK 8096
#define BAT_TASK_PRI 1
#define BAT_TASK_STACK 2048
#define WIFI_TASK_PRI 1
#define WIFI_TASK_STACK 2048
#define LOGGER_TASK_PRI 1
#define LOGGER_TASK_STACK 4096


void setup() {
  setCpuFrequencyMhz(240);

  // Init serial for logging
  Serial.begin(115200);

  // Create FreeRTOS queue
  samples_queue = xQueueCreate(8, sizeof(float));

  initColorPins();

  // Create the mic reader task and pin it to the first core (ID=0)
  xTaskCreatePinnedToCore(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL, 0);

  // Create the Leq calculator task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(leq_calculator_task, "Leq Calculator", LEQ_TASK_STACK, NULL, LEQ_TASK_PRI, NULL, 1);

  #ifdef USE_BATTERY
  xTaskCreatePinnedToCore(battery_checker_task, "Battery Checker", BAT_TASK_STACK, NULL, BAT_TASK_PRI, NULL, 1);
  #endif

  #ifdef USE_LOGGING
  // Create the WiFi connection task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(wifi_checker_task, "WiFi Checker", WIFI_TASK_STACK, NULL, WIFI_TASK_PRI, NULL, 1);
  
  // Initialize the logging queue
  logging_queue = xQueueCreate(10, sizeof(LogData));
  // Create the logger task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(logger_task, "Logger", LOGGER_TASK_STACK, NULL, LOGGER_TASK_STACK, NULL, 1);
  #endif

  vTaskDelay(pdMS_TO_TICKS(1000)); // Safety
}

void loop() {
  // Execution will never reach this point since we're using FreeRTOS
}