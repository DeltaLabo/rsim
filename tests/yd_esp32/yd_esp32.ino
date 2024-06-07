#include <WiFi.h>
#include <ThingSpeak.h>
#include <Arduino.h>
#include <cstring>
#include <esp_now.h>
#include <HardwareSerial.h>
#include <esp_system.h>
#include "time.h"

#include "rtc.h"
#include "driver/i2s.h"
#include "slm_params.h"
#include "sos-iir-filter-xtensa.h"

// Equalizer used to flatten the microphone's frequency response
#define MIC_EQUALIZER     INMP441

// Values taken from microphone datasheet
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

// I2S peripheral to use (0 or 1)
#define I2S_PORT          I2S_NUM_0

#ifdef XIAO
#define I2S_WS  D2
#define I2S_SCK D3
#define I2S_SD  D8
#else
#ifdef YD
#define I2S_WS  13
#define I2S_SCK 12
#define I2S_SD  11
#endif
#endif

#ifdef XIAO
#define RED_LED_PIN D0 
#define GREEN_LED_PIN D1
#ifdef USE_BLUE_LED
#define BLUE_LED_PIN D9
#endif
#else
#ifdef YD
#define RED_LED_PIN 21
#define GREEN_LED_PIN 35
#ifdef USE_BLUE_LED
#define BLUE_LED_PIN 20
#endif
#endif
#endif

#define GREEN_LED_CHANNEL 0
#define RED_LED_CHANNEL 1
#ifdef USE_BLUE_LED
#define BLUE_LED_CHANNEL 2
#endif

#define LED_PWM_FREQ 5000 // Hz
#define LED_PWM_RES 8 // Bits

#ifdef XIAO
#define SDA_PIN 4
#define SCL_PIN 5
#else
#ifdef YD
#define SDA_PIN 5
#define SCL_PIN 15
#endif
#endif

#ifdef XIAO
#define RX_PIN D7
#define TX_PIN D6
#else
#ifdef YD
#define RX_PIN 44
#define TX_PIN 43
#endif
#endif

// HardwareSerial for logging
HardwareSerial HwSerial(0);

// WiFi client used for ThingSpeak logging
WiFiClient  client;

// Flag to determine when to log measurements
// Works as a counter of how many SLM periods have passed from the last logging event
short logFlag = 0;

// Variable to store the ThingSpeak error code returned after sending data
int thingSpeakErrorCode;

// Variable to store ESPNOW data transmission result
String success;

typedef struct espnow_message {
  int currentColor;
} espnow_message;

espnow_message localReadings;
espnow_message incomingReadings;

// Previous local color indication
// Used to avoid setting the LEDs to the same color
// they already were
// Initialized to a null value
int prevColor = -1;

TickType_t lastReceivedTime = 0;
int syncFailureCounter = 0;

esp_now_peer_info_t peerInfo;

int brightness = 255;
int gBright = 0;
int rBright = 0;
int bBright = 0;
int fadeSpeed = 5;

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
#define SAMPLE_T          int32_t // Sample data type, 32 bits to accommodate the incoming 24-bit samples
#define SAMPLES_SHORT     (SAMPLE_RATE / 8) // ~125ms
#define SAMPLES_LEQ       (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE     (SAMPLES_SHORT / 16)
#define DMA_BANKS         32

// Data pushed to 'samples_queue'
struct sum_queue_t {
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
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
// FreeRTOS priority and stack sizes (in 32-bit words) 
#define I2S_TASK_PRI   4
#define I2S_TASK_STACK 2048
#define LEQ_TASK_PRI   3
#define LEQ_TASK_STACK 8096
#define RTC_TASK_PRI 1
#define RTC_TASK_STACK 2048

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
    
    // Convert (including shifting) integer microphone values to floats, 
    // using the same buffer (assumed sample size is same as size of float), 
    // to save a bit of memory
    SAMPLE_T* int_samples = (SAMPLE_T*)&samples;
    for(int i=0; i<SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);

    sum_queue_t q;
    // Apply DC blocker,
    // writes filtered samples back to the same buffer.
    q.sum_sqr_SPL = DC_BLOCKER.filter(samples, samples, SAMPLES_SHORT);

    // Apply equalization and calculate Z-weighted sum of squares
    q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

    // Apply weighting and calculate weigthed sum of squares
    q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

    // Send the sums to FreeRTOS queue where main task will pick them up
    // and further calcualte decibel values (division, logarithms, etc...)
    xQueueSend(samples_queue, &q, portMAX_DELAY);
  }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  status == ESP_NOW_SEND_SUCCESS;
  if (status == 0){
    HwSerial.println("[INFO] [ESPNOW]: Delivery success.");
  }
  else{
    HwSerial.println("[ERROR] [ESPNOW]: Delivery fail.");
  }
}

void logToThingSpeak(double Logging_leq, double Max_leq, double Min_leq) {
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    HwSerial.print("[INFO] [THINGSPEAK]: Attempting to connect to WIFI...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (WiFi.status() == WL_CONNECTED){
    // Write to ThingSpeak
    // Field 1: Equivalent noise level for the entire time period (Local)
    ThingSpeak.setField(1, float(Logging_leq));
    // Field 2: Maximum indiviual measurement within the time period (Local)
    ThingSpeak.setField(2, float(Max_leq));
    // Field 3: Minimum indiviual measurement within the time period (Local)
    ThingSpeak.setField(3, float(Min_leq));

    // Params: Channel ID, Write API key
    thingSpeakErrorCode = ThingSpeak.writeFields(CHANNEL_NUMBER, WRITE_API_KEY);

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

    // Print ThingSpeak error code to HwSerial
    if(thingSpeakErrorCode == 200){
      HwSerial.println("[INFO] [THINGSPEAK]: Channel update successful.");
    }
    else{
      HwSerial.println("[ERROR] [THINGSPEAK]: Problem updating channel. HTTP error code " + String(thingSpeakErrorCode));
    }
  }
  else {
    HwSerial.println("[ERROR] [THINGSPEAK]: Could not connect to WiFi.");
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  lastReceivedTime = xTaskGetTickCountFromISR();
  HwSerial.print("[INFO] [ESPNOW]: Data received, Color: ");
  HwSerial.println(incomingReadings.currentColor);
}

void leq_calculator_task(void* parameter) {
  // Queue object for microphone data
  sum_queue_t q;
  // Counter for the amount of samples read in a single measurement period
  uint32_t Leq_samples = 0;
  // Counter for the amount of samples read in a single logging period
  uint32_t Logging_samples = 0;
  // Variable to store the sum of squares of a single measurement batch
  double Leq_sum_sqr = 0;
  // Variable to store the sum of squares of a single logging batch
  double Logging_sum_sqr = 0;
  // Final noise level value in dB (with the selected weighting applied)
  double Leq_dB = 0;
  // Final noise level value for one logging period in dB (with the selected weighting applied)
  double Logging_leq = 0;
  // Maximum noise level within one logging period
  // Initialized to a value that's lower than any possible measurement
  double Max_leq = MIC_NOISE_DB - 1.0;
  // Minimum noise level within one logging period
  // Initialized to a value that's higher than any possible measurement
  double Min_leq = MIC_OVERLOAD_DB + 1.0;

  // Read sum of samples, calculated by 'i2s_reader_task'
  while (xQueueReceive(samples_queue, &q, portMAX_DELAY)) {
    // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
    double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
    double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

    // Accumulate Leq sum
    Leq_sum_sqr += q.sum_sqr_weighted;
    // Update the amount of samples read
    Leq_samples += SAMPLES_SHORT;

    // When we gather enough samples, calculate new Leq value 
    if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
      // Calculate RMS of the equivalent sound level
      double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
      // Calculate sound level in decibels, with respect to the microphone reference
      Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);

      // In case of acoustic overload or below noise floor measurement, report limit Leq value
      if (Leq_dB > MIC_OVERLOAD_DB) Leq_dB = MIC_OVERLOAD_DB;
      else if (Leq_dB < MIC_NOISE_DB) Leq_dB = MIC_NOISE_DB;

      // Update max and min values, if needed
      if (Leq_dB < Min_leq) Min_leq = Leq_dB;
      if (Leq_dB > Max_leq) Max_leq = Leq_dB;

      updateColor(Leq_dB);
      HwSerial.print("[INFO] [SLM]: Local reading: ");
      HwSerial.print(Leq_dB);
      HwSerial.print(", Color: ");
      HwSerial.println(localReadings.currentColor);
      
      // ThingSpeak data calculation
      // Accumulate Leq sum
      Logging_sum_sqr += Leq_sum_sqr;
      // Update the amount of samples read
      Logging_samples += Leq_samples;

      // When a new dB measurement is ready
      if (Logging_samples >= SAMPLE_RATE * LOGGING_PERIOD) {
        // Calculate RMS of the equivalent sound level for the entire logging period
        double Logging_leq_RMS = sqrt(Logging_sum_sqr / Logging_samples);
        // Calculate sound level in decibels, with respect to the microphone reference
        Logging_leq = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Logging_leq_RMS / MIC_REF_AMPL);

        // In case of acoustic overload or below noise floor measurement, report limit Leq value
        if (Logging_leq > MIC_OVERLOAD_DB) Logging_leq = MIC_OVERLOAD_DB;
        else if (Logging_leq < MIC_NOISE_DB) Logging_leq = MIC_NOISE_DB;

        #ifdef USE_THINGSPEAK
        logToThingSpeak(Logging_leq, Max_leq, Min_leq);
        #endif

        // Reset the sum of squares and sample counter
        Logging_sum_sqr = 0;
        Logging_samples = 0;
        // Reset max and min values
        Max_leq = MIC_NOISE_DB - 1.0;
        Min_leq = MIC_OVERLOAD_DB + 1.0;
      }

      // Reset the sum of squares and sample counter
      Leq_sum_sqr = 0;
      Leq_samples = 0;

      #ifdef USE_ESPNOW
      // ESP-NOW comms
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &localReadings, sizeof(localReadings));

      TickType_t latency = 0;

      vTaskDelay(pdMS_TO_TICKS(30));
      latency = xTaskGetTickCount() - lastReceivedTime;

      if (latency <= pdMS_TO_TICKS(MAX_LATENCY)) {
        HwSerial.print("[INFO] [ESPNOW]: Incoming reading received on time, Color: ");
        HwSerial.print(incomingReadings.currentColor);
        HwSerial.print(", Latency: ");
        HwSerial.println(latency);

        syncFailureCounter = 0;

        if (incomingReadings.currentColor > localReadings.currentColor)
        {
          setLEDColor(incomingReadings.currentColor);
        }
        else{
          setLEDColor(localReadings.currentColor);  
        }
      }
      else if (latency <= pdMS_TO_TICKS(SYNC_FAIL_LATENCY)) {
        syncFailureCounter++;

        if (syncFailureCounter >= MAX_SYNC_FAILURES) {
          HwSerial.println("[ERROR] [ESPNOW]: Sync failures have exceeded the limit. Restarting ESP.");
          restart_esp(NULL);
        }

        HwSerial.print("[ERROR] [ESPNOW]: Incoming reading was not received on time, Color: ");
        HwSerial.print(incomingReadings.currentColor);
        HwSerial.print(", Latency: ");
        HwSerial.print(latency);
        HwSerial.print(", Failures: ");
        HwSerial.println(syncFailureCounter);

        setLEDColor(localReadings.currentColor);
      }
      else {
        HwSerial.println("[ERROR] [ESPNOW]: Did not receive any measurements. There doesn't seem to be another RSIM device active within range.");

        setLEDColor(localReadings.currentColor);
      }
      #else
      setLEDColor(localReadings.currentColor);
      #endif
    }
  }
}

void restart_esp(TimerHandle_t xTimer) {
  esp_restart();
}

void Update_RTC(TimerHandle_t xTimer) {
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    HwSerial.println("[INFO] [RTC]: Attempting to connect to WIFI...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (WiFi.status() == WL_CONNECTED){
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm_bytes timeInfo;

    if(getLocalTimeinBytes(&timeInfo)){
      setDS3231time(
        timeInfo.tm_sec,
        timeInfo.tm_min,
        timeInfo.tm_hour,
        timeInfo.tm_wday,
        timeInfo.tm_mday,
        timeInfo.tm_mon,
        timeInfo.tm_year
      );
      HwSerial.println("[INFO] [RTC]: RTC time updated.");
    }
    else HwSerial.println("[ERROR] [RTC]: Could not update RTC time.");
  }
  else HwSerial.println("[ERROR] [RTC]: Could not connect to WiFi.");
}

void awaitEvenSecond() {
  HwSerial.println("[INFO] [RTC]: Awaiting even second...");
  byte s0, s1;
  // Initial time measurement
  readDS3231seconds(&s0);
  HwSerial.print("initial: ");
  HwSerial.println(s0);

  // Wait until next second change
  do {
    readDS3231seconds(&s1);
    HwSerial.println(s1);
  }
  while(s0 == s1);

  HwSerial.println("second changed");
  
  // Wait until next even second
  do {
    readDS3231seconds(&s1);
    HwSerial.println(s1);
  }
  while(s1 % 2 != 0);

  HwSerial.println("[INFO] [RTC]: Ready to start.");
}

void RTC_update_handler_task(void* parameter) {
  #ifdef USE_RTC
  // Create a software timer for RTC updates
  TimerHandle_t UpdateTimer = xTimerCreate("RTC Update Timer",      // Timer name
                                      pdMS_TO_TICKS(RTC_UPDATE_PERIOD), // Timer period
                                      pdTRUE,              // Auto-reload
                                      NULL,                // Timer ID
                                      Update_RTC);      // Callback function

  if (UpdateTimer != NULL) {
      // Start the timer
      xTimerStart(UpdateTimer, 0);
  }
  #endif

  // Create a software timer to restart the ESP32
  TimerHandle_t RestartTimer = xTimerCreate("ESP32 Restart Timer",      // Timer name
                                    pdMS_TO_TICKS(ESP32_RESTART_PERIOD), // Timer period
                                    pdTRUE,              // Auto-reload
                                    NULL,                // Timer ID
                                    restart_esp);      // Callback function

  if (RestartTimer != NULL) {
      // Start the timer
      xTimerStart(RestartTimer, 0);
  }

  // Task code can continue here...

  // Task should not exit
  while(true) {
      vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 milliseconds
  }
}

// Update the color indication based on the Leq value
void updateColor(float Leq_dB){
  if (Leq_dB < GREEN_UPPER_LIMIT) {
    localReadings.currentColor = GREEN;
  }
  else if (Leq_dB < YELLOW_UPPER_LIMIT) {
    localReadings.currentColor = YELLOW;
  }
  else {
    localReadings.currentColor = RED;
  }
}

// Update the LED color
void setLEDColor(int color){
  if (color != prevColor) {
    prevColor = color;
    ledcWrite(GREEN_LED_CHANNEL, 0);
    ledcWrite(RED_LED_CHANNEL, 0);
    #ifdef USE_BLUE_LED
    ledcWrite(BLUE_LED_CHANNEL, 0);
    #endif

    if(color == RED){
      ledcWrite(RED_LED_CHANNEL, 255);
    }
    else if(color == GREEN){
      ledcWrite(GREEN_LED_CHANNEL, 255);
    }
    else { // color == YELLOW
      ledcWrite(GREEN_LED_CHANNEL, 255);
      ledcWrite(RED_LED_CHANNEL, 25);
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
  setCpuFrequencyMhz(240);  

  // Set up ledc (PWM) channels
  ledcSetup(GREEN_LED_CHANNEL, LED_PWM_FREQ, LED_PWM_RES);
  ledcAttachPin(GREEN_LED_PIN, GREEN_LED_CHANNEL);
  ledcSetup(RED_LED_CHANNEL, LED_PWM_FREQ, LED_PWM_RES);
  ledcAttachPin(RED_LED_PIN, RED_LED_CHANNEL);
  #ifdef USE_BLUE_LED
  ledcSetup(BLUE_LED_CHANNEL, LED_PWM_FREQ, LED_PWM_RES);
  ledcAttachPin(BLUE_LED_PIN, BLUE_LED_CHANNEL);
  #endif

  Wire.begin(SDA_PIN, SCL_PIN);

  HwSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Create FreeRTOS queue
  samples_queue = xQueueCreate(8, sizeof(sum_queue_t));

  #ifdef USE_THINGSPEAK
  // Initialize the ThingSpeak object with the required WiFi client
  ThingSpeak.begin(client);
  #endif

  // Required to use IEEE 802.11 WiFi standard and ESPNOW
  #if defined(USE_ESPNOW) || defined(USE_THINGSPEAK)
  HwSerial.println("before");
  WiFi.mode(WIFI_STA);
  HwSerial.println("after");
  #endif

  int espnowInitCounter = 0;

  #ifdef USE_ESPNOW
  // Init ESPNOW
  while (esp_now_init() != ESP_OK && espnowInitCounter < ESPNOW_MAX_INIT_FAILURES) {
    espnowInitCounter++;
    HwSerial.print("[ERROR] [ESPNOW]: Could not initialize ESP-NOW. Failures: ");
    HwSerial.println(espnowInitCounter);
  }

  if (!(espnowInitCounter < ESPNOW_MAX_INIT_FAILURES)) {
    HwSerial.print("[ERROR] [ESPNOW]: ESP-NOW initialization failures have exceeded the limit. Restarting ESP. ");
    restart_esp(NULL);
  }
  else espnowInitCounter = 0;

  // Register ESPNOW peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  while (esp_now_add_peer(&peerInfo) != ESP_OK && espnowInitCounter < ESPNOW_MAX_INIT_FAILURES){
    espnowInitCounter++;
    HwSerial.print("[ERROR] [ESPNOW]: Failed to add peer. Failures: ");
    HwSerial.println(espnowInitCounter);
  }

  if (!(espnowInitCounter < ESPNOW_MAX_INIT_FAILURES)) {
    HwSerial.println("[ERROR] [ESPNOW]: ESP-NOW initialization failures have exceeded the limit. Restarting ESP. ");
    restart_esp(NULL);
  }
  else espnowInitCounter = 0;

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  #endif

  #ifdef USE_RTC
  Update_RTC(NULL);
  awaitEvenSecond();
  #endif

  #ifndef USE_RTC
  HwSerial.println("[INFO] [RTC]: The RTC is disabled.");
  #endif

  #ifndef USE_ESPNOW
  HwSerial.println("[INFO] [ESPNOW]: ESP-NOW is disabled.");
  #endif

  #ifndef USE_THINGSPEAK
  HwSerial.println("[INFO] [THINGSPEAK]: ThingSpeak logging is disabled.");
  #endif

  // Create the RTC update task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(RTC_update_handler_task, "RTC update handler", RTC_TASK_STACK, NULL, RTC_TASK_PRI, NULL, 1);

  // Create the mic reader task and pin it to the first core (ID=0)
  xTaskCreatePinnedToCore(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL, 0);

  // Create the Leq calculator task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(leq_calculator_task, "Leq Calculator", LEQ_TASK_STACK, NULL, LEQ_TASK_PRI, NULL, 1);
  
  vTaskDelay(pdMS_TO_TICKS(1000)); // Safety
}

void loop() {
  // Execution will never reach this point since we're using FreeRTOS
}
