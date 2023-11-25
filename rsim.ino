#include <WiFi.h>
#include <ThingSpeak.h>
#include <Arduino.h>
#include <cstring>
#include "LEDStripDriver.h"

#include <driver/i2s.h>
#include "slm_params.h"
#include "sos-iir-filter-xtensa.h"

#define WIFI_SSID "LaboratorioDelta"
#define WIFI_PASSWORD "labdelta21!"
#define WRITE_API_KEY "7MF9B7PO9N1OF3HF"
#define CHANNEL_NUMBER 2354933

//
// Configuration
//

// Measurement display toggle
#define WIFI 0
#define SERIAL 1
#define WIFI_PLUS_SERIAL 2
#define LOG_MODE WIFI

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER     INMP441    // See below for defined IIR filters or set to 'None' to disable

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY   -26         // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB        94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB   120.0       // dB - Acoustic overload point
#define MIC_NOISE_DB      33.0        // dB - Noise floor
#define MIC_BITS          24          // valid number of bits in I2S data
#define MIC_CONVERT(s)    (s >> (SAMPLE_BITS - MIC_BITS))
#define MIC_TIMING_SHIFT  0           // Set to one to fix MSB timing for some microphones, i.e. SPH0645LM4H-x

// Calculate reference amplitude value at compile time
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY)/20) * ((1<<(MIC_BITS-1))-1);

//
// I2S pins - Can be routed to almost any (unused) ESP32 pin.
//            SD can be any pin, inlcuding input only pins (36-39).
//            SCK (i.e. BCLK) and WS (i.e. L/R CLK) must be output capable pins
#define I2S_WS D2
#define I2S_SD D8
#define I2S_SCK D3

// LED indicator toggle
#define USE_LED_INDICATOR 1

// I2S peripheral to use (0 or 1)
#define I2S_PORT          I2S_NUM_0

// DIN=GPIO6 (Pin 5) CIN=GPIO7  (Pin 6)
LEDStripDriver led = LEDStripDriver(5, 6);

WiFiClient  client;

#define GREEN 0
#define YELLOW 1
#define RED 2
short currentColor = -1;
bool switchFlag = false;

short logFlag = 0;

int thingSpeakErrorCode;

//
// IIR Filters
//

// DC-Blocker filter - removes DC component from I2S data
// See: https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
// a1 = -0.9992 should heavily attenuate frequencies below 10Hz
SOS_IIR_Filter DC_BLOCKER = { 
  gain: 1.0,
  sos: {{-1.0, 0.0, +0.9992, 0}}
};

// 
// Equalizer IIR filters to flatten microphone frequency response
// See respective .m file for filter design. Fs = 48Khz.
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
  gain: 1.00197834654696, 
  sos: { // Second-Order Sections {b1, b2, -a1, -a2}
    {-1.986920458344451, +0.986963226946616, +1.995178510504166, -0.995184322194091}
  }
};


//
// Weighting filters
//

//
// A-weighting IIR Filter, Fs = 48KHz 
// (By Dr. Matt L., Source: https://dsp.stackexchange.com/a/36122)
// B = [0.169994948147430, 0.280415310498794, -1.120574766348363, 0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003]
// A = [1.0, -2.12979364760736134, 0.42996125885751674, 1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968]
SOS_IIR_Filter A_weighting = {
  gain: 0.169994948147430, 
  sos: { // Second-Order Sections {b1, b2, -a1, -a2}
    {-2.00026996133106, +1.00027056142719, -1.060868438509278, -0.163987445885926},
    {+4.35912384203144, +3.09120265783884, +1.208419926363593, -0.273166998428332},
    {-0.70930303489759, -0.29071868393580, +1.982242159753048, -0.982298594928989}
  }
};

//
// C-weighting IIR Filter, Fs = 48KHz 
// Designed by invfreqz curve-fitting
// B = [-0.49164716933714026, 0.14844753846498662, 0.74117815661529129, -0.03281878334039314, -0.29709276192593875, -0.06442545322197900, -0.00364152725482682]
// A = [1.0, -1.0325358998928318, -0.9524000181023488, 0.8936404694728326   0.2256286147169398  -0.1499917107550188, 0.0156718181681081]
SOS_IIR_Filter C_weighting = {
  gain: -0.491647169337140,
  sos: { 
    {+1.4604385758204708, +0.5275070373815286, +1.9946144559930252, -0.9946217070140883},
    {+0.2376222404939509, +0.0140411206016894, -1.3396585608422749, -0.4421457807694559},
    {-2.0000000000000000, +1.0000000000000000, +0.3775800047420818, -0.0356365756680430}
  }
};


//
// Sampling
//
#define SAMPLE_RATE       48000 // Hz, fixed to design of IIR filters
#define SAMPLE_BITS       32    // bits
#define SAMPLE_T          int32_t 
#define SAMPLES_SHORT     (SAMPLE_RATE / 8) // ~125ms
#define SAMPLES_LEQ       (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE     (SAMPLES_SHORT / 16)
#define DMA_BANKS         32

// Data we push to 'samples_queue'
struct sum_queue_t {
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
  // Debug only, FreeRTOS ticks we spent processing the I2S data
  uint32_t proc_ticks;
};
QueueHandle_t samples_queue;

// Static buffer for block of samples
float samples[SAMPLES_SHORT] __attribute__((aligned(4)));


//
// I2S Microphone sampling setup 
//
void mic_i2s_init() {
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  const i2s_config_t i2s_config = {
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_CHANNEL_FMT_ONLY_LEFT,
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0
  };
  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    bck_io_num:   I2S_SCK,  
    ws_io_num:    I2S_WS,    
    data_out_num: -1, // not used
    data_in_num:  I2S_SD   
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  #if (MIC_TIMING_SHIFT > 0) 
    // Undocumented (?!) manipulation of I2S peripheral registers
    // to fix MSB timing issues with some I2S microphones
    REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));   
    REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);  
  #endif
  
  i2s_set_pin(I2S_PORT, &pin_config);
}

//
// I2S Reader Task
//
// Rationale for separate task reading I2S is that IIR filter
// processing cam be scheduled to different core on the ESP32
// while main task can do something else
//
// As this is intended to run as separate high-priority task, 
// we only do the minimum required work with the I2S data
// until it is 'compressed' into sum of squares 
//
// FreeRTOS priority and stack size (in 32-bit words) 
#define I2S_TASK_PRI   4
#define I2S_TASK_STACK 2048
//
void mic_i2s_reader_task(void* parameter) {
  mic_i2s_init();

  // Discard first block, microphone may have startup time (i.e. INMP441 up to 83ms)
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);

  while (true) {
    // Block and wait for microphone values from I2S
    //
    // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
    // and when there is requested ammount of data, task is unblocked
    //
    // Note: i2s_read does not care it is writing in float[] buffer, it will write
    //       integer values to the given address, as received from the hardware peripheral. 
    i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

    TickType_t start_tick = xTaskGetTickCount();
    
    // Convert (including shifting) integer microphone values to floats, 
    // using the same buffer (assumed sample size is same as size of float), 
    // to save a bit of memory
    SAMPLE_T* int_samples = (SAMPLE_T*)&samples;
    for(int i=0; i<SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);

    sum_queue_t q;
    // Apply equalization and calculate Z-weighted sum of squares,
    // writes filtered samples back to the same buffer.
    q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

    // Apply weighting and calucate weigthed sum of squares
    q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);


    // Debug only. Ticks we spent filtering and summing block of I2S data
    q.proc_ticks = xTaskGetTickCount() - start_tick;

    // Send the sums to FreeRTOS queue where main task will pick them up
    // and further calcualte decibel values (division, logarithms, etc...)
    xQueueSend(samples_queue, &q, portMAX_DELAY);
  }
}

// Update the LED indicators depending on Leq value
void updateLEDColor(float Leq_dB){
  if (currentColor == -1) {
    switchFlag = false;
    // Turn the Green LED on if Leq is less than 50 dB
    if (Leq_dB < GREEN_UPPER_LIMIT) {
      led.setColor(0, 255, 0); // Verde
      currentColor = GREEN;
    }
    // Turn the Yellow LED on if Leq is less than 70 dB
    else if (Leq_dB < YELLOW_UPPER_LIMIT) {
      led.setColor(255, 255, 0); // Amarillo
      currentColor = YELLOW;
    }
    // Turn the Red LED on if Leq is greater than or equal to 70 dB
    else {
      led.setColor(255, 0, 0); // Rojo
      currentColor = RED;
    }
  }
  else if (currentColor == GREEN) {
    if (Leq_dB < GREEN_UPPER_LIMIT) {
      switchFlag = false;
    }
    // Turn the Yellow LED on if Leq is up to 70 dB
    else if (Leq_dB >= GREEN_UPPER_LIMIT && (Leq_dB < YELLOW_UPPER_LIMIT) && switchFlag == true) {
      led.setColor(255, 255, 0); // Amarillo
      currentColor = YELLOW;
      switchFlag = false;
    }
    else if (Leq_dB >= GREEN_UPPER_LIMIT && (Leq_dB < YELLOW_UPPER_LIMIT && switchFlag == false)) {
      switchFlag = true;
    }
    // Turn the Red LED on if Leq is greater than or equal to 70 dB
    else if (Leq_dB >= YELLOW_UPPER_LIMIT && switchFlag == true) {
      led.setColor(255, 0, 0); // Rojo
      currentColor = RED;
      switchFlag = false;
    }
    else if (Leq_dB >= YELLOW_UPPER_LIMIT && switchFlag == false) {
      switchFlag = true;
    }
  }
  else if (currentColor == YELLOW) {
    // Turn the Green LED on if Leq is less than 50 dB
    if (Leq_dB < GREEN_UPPER_LIMIT) {
      led.setColor(0, 255, 0); // Verde
      currentColor = GREEN;
      switchFlag = true;
    }
    else if (Leq_dB < YELLOW_UPPER_LIMIT) {
      switchFlag = true;
    }
    else if (switchFlag == true) {
      led.setColor(255, 0, 0); // Rojo
      currentColor = RED;
    }
    else {
      switchFlag = true;
    }
  }
  else if (currentColor == RED) {
    switchFlag = true;
    // Turn the Green LED on if Leq is less than 50 dB
    if (Leq_dB < GREEN_UPPER_LIMIT) {
      led.setColor(0, 255, 0); // Verde
      currentColor = GREEN;
    }
    // Turn the Yellow LED on if Leq is less than 70 dB
    else if (Leq_dB < YELLOW_UPPER_LIMIT) {
      led.setColor(255, 255, 0); // Amarillo
      currentColor = YELLOW;
    }
    // Turn the Red LED on if Leq is greater than or equal to 70 dB
    else {
      led.setColor(255, 0, 0); // Rojo
      currentColor = RED;
    }
  }
}

//
// Setup and main loop 
//
// Note: Use doubles, not floats, here unless you want to pin
//       the task to whichever core it happens to run on at the moment
// 
void setup() {
  setCpuFrequencyMhz(230);

  // Create FreeRTOS queue
  samples_queue = xQueueCreate(8, sizeof(sum_queue_t));
  
  // Create the mic reader task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL, 1);

  if (LOG_MODE == SERIAL) Serial.begin(115200);
  else if (LOG_MODE == WIFI) {
    WiFi.mode(WIFI_STA);   
    ThingSpeak.begin(client);
  }
  else if (LOG_MODE == WIFI_PLUS_SERIAL)
  {
    Serial.begin(115200); // TODO: delete
    WiFi.mode(WIFI_STA);   
    ThingSpeak.begin(client);
  }

  sum_queue_t q;
  uint32_t Leq_samples = 0;
  double Leq_sum_sqr = 0;
  double Leq_dB = 0;

  delay(1000); // Safety

  // Read sum of samples, calculated by 'i2s_reader_task'
  while (xQueueReceive(samples_queue, &q, portMAX_DELAY)) {

    // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
    double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
    double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

    // In case of acoustic overload or below noise floor measurement, report limit Leq value
    /*
    if (short_SPL_dB > MIC_OVERLOAD_DB) {
      Leq_sum_sqr = INFINITY;
    } else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
      Leq_sum_sqr = -INFINITY;
    }
    */

    // Accumulate Leq sum
    Leq_sum_sqr += q.sum_sqr_weighted;
    Leq_samples += SAMPLES_SHORT;

    // When we gather enough samples, calculate new Leq value
    if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
      double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
      Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
      Leq_sum_sqr = 0;
      Leq_samples = 0;

      // In case of acoustic overload or below noise floor measurement, report limit Leq value
      if (Leq_dB > MIC_OVERLOAD_DB) Leq_dB = MIC_OVERLOAD_DB;
      else if (Leq_dB < MIC_NOISE_DB) Leq_dB = MIC_NOISE_DB;
      
      // Serial output, customize (or remove) as needed
      if (LOG_MODE == SERIAL) Serial.printf("%.1f %s\n", Leq_dB, DB_UNITS);
      else if (LOG_MODE == WIFI) {
        if (USE_LED_INDICATOR == 1) updateLEDColor(Leq_dB);

        if (logFlag == 9) {
          logFlag = 0;
          // Connect or reconnect to WiFi
          if(WiFi.status() != WL_CONNECTED){
            Serial.print("Attempting to connect to WIFI...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
            vTaskDelay(pdMS_TO_TICKS(5000)); // 5000 ms
            if (WiFi.status() != WL_CONNECTED){
              Serial.println(" Couldn't connect.");
            }
            else Serial.println(" Connected.");
          }

          if (WiFi.status() == WL_CONNECTED){
            // Write to ThingSpeak.
            // Params: Channel ID, Field Number, Value, Write API key
            thingSpeakErrorCode = ThingSpeak.writeField(CHANNEL_NUMBER, 1, float(Leq_dB), WRITE_API_KEY);

            /*
            Possible response codes:
            200 - OK / Success
            404 - Incorrect API key (or invalid ThingSpeak server address)
            -101 - Value is out of range or string is too long (> 255 characters)
            -201 - Invalid field number specified
            -210 - setField() was not called before writeFields()
            -301 - Failed to connect to ThingSpeak
            -302 -  Unexpected failure during write to ThingSpeak
            -303 - Unable to parse response
            -304 - Timeout waiting for server to respond
            -401 - Point was not inserted (most probable cause is exceeding the rate limit)
            */

            if(thingSpeakErrorCode == 200){
              Serial.println("Channel update successful.");
            }
            else{
              Serial.println("Problem updating channel. HTTP error code " + String(thingSpeakErrorCode));
            }
          }     
        }
        else logFlag = logFlag + 1;
      }
      else if (LOG_MODE == WIFI_PLUS_SERIAL) {
        Serial.printf("%.1f %s\n", Leq_dB, DB_UNITS); // TODO: delete
        if (USE_LED_INDICATOR == 1) updateLEDColor(Leq_dB);

        if (logFlag == 9) {
          logFlag = 0;

          // Connect or reconnect to WiFi
          if(WiFi.status() != WL_CONNECTED){
            Serial.print("Attempting to connect to WIFI...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
            vTaskDelay(pdMS_TO_TICKS(5000)); // 5000 ms
            if (WiFi.status() != WL_CONNECTED){
              Serial.println(" Couldn't connect.");
            }
            else Serial.println(" Connected.");
          }

          if (WiFi.status() == WL_CONNECTED){
            // Write to ThingSpeak.
            // Params: Channel ID, Field Number, Value, Write API key
            thingSpeakErrorCode = ThingSpeak.writeField(CHANNEL_NUMBER, 1, float(Leq_dB), WRITE_API_KEY);

            if(thingSpeakErrorCode == 200){
              Serial.println("Channel update successful.");
            }
            else{
              Serial.println("Problem updating channel. HTTP error code " + String(thingSpeakErrorCode));
            }
          }
        }
        else logFlag = logFlag + 1;
      }

      // Debug only
      //Serial.printf("%u processing ticks\n", q.proc_ticks);
    }
  }
}

void loop() {
  // Nothing here..
}
