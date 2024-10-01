#include <Arduino.h>
#include <cstring>
#include "time.h"
#include "freertos/semphr.h"
#include <Wire.h>
#include <Adafruit_INA219.h>


#include "driver/i2s.h"
#include "slm_params.h"
#include "sos-iir-filter-xtensa.h"
#include "pins.h"

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

// Previous local color indication
// Used to avoid setting the LEDs to the same color
// they already were
// Initialized to a null value
int prevColor = -1;

int currentColor = 0;

// Battery voltage and current meter
Adafruit_INA219 ina219;

short batteryState = ENOUGH_BATTERY;

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
#define BAT_TASK_PRI 1
#define BAT_TASK_STACK 2048

void mic_i2s_reader_task(void* parameter) {
  mic_i2s_init();

  // Discard first block, microphone may have startup time (i.e. INMP441 up to 83ms)
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);

  while (true) {
    #if defined(USE_ESPNOW) && defined(ESPNOW_CLIENT)
    xSemaphoreTake(xMutex, portMAX_DELAY);
    if (syncStatus == FREEZE) {
      xSemaphoreGive(xMutex);
      vTaskDelay(incomingReadings.latency);
      xSemaphoreTake(xMutex, portMAX_DELAY);
      syncStatus = SYNCING;
      xSemaphoreGive(xMutex);
    }
    else xSemaphoreGive(xMutex);
    #endif

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

    // Apply DC blocker,
    // writes filtered samples back to the same buffer.
    float sum_sqr_weighted = DC_BLOCKER.filter(samples, samples, SAMPLES_SHORT);

    // Apply equalization and calculate Z-weighted sum of squares
    sum_sqr_weighted = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

    // Apply weighting and calculate weigthed sum of squares
    sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

    // Send the sums to FreeRTOS queue where main task will pick them up
    // and further calcualte decibel values (division, logarithms, etc...)
    xQueueSend(samples_queue, &sum_sqr_weighted, portMAX_DELAY);
  }
}

void leq_calculator_task(void* parameter) {
  // Queue object for microphone data
  float sum_sqr_weighted;
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
  while (true) {
    if (xQueueReceive(samples_queue, &sum_sqr_weighted, portMAX_DELAY)) {
      // Accumulate Leq sum
      Leq_sum_sqr += sum_sqr_weighted;
      // Update the amount of samples read
      Leq_samples += SAMPLES_SHORT;
    }

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
      setLEDColor(currentColor);
      Serial.print("[INFO] [SLM]: Local reading: ");
      Serial.print(Leq_dB);
      Serial.print(", Color: ");
      Serial.println(currentColor);

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
    }
  }
}

void battery_checker_task(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;

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

// Update the color indication based on the Leq value
void updateColor(float Leq_dB){
  if (Leq_dB < GREEN_UPPER_LIMIT) {
    currentColor = GREEN;
  }
  else if (Leq_dB < YELLOW_UPPER_LIMIT) {
    currentColor = YELLOW;
  }
  else {
    currentColor = RED;
  }
}

// Update the LED color
void setLEDColor(int color){
  if (color != prevColor) {
    prevColor = color;
    ledcWrite(GREEN_LED_CHANNEL, 255);
    ledcWrite(RED_LED_CHANNEL, 255);
    ledcWrite(BLUE_LED_CHANNEL, 255);

    if(color == RED){
      ledcWrite(RED_LED_CHANNEL, 0);
    }
    else if(color == GREEN){
      ledcWrite(GREEN_LED_CHANNEL, 0);
    }
    else { // color == YELLOW
      ledcWrite(GREEN_LED_CHANNEL, 100);
      ledcWrite(RED_LED_CHANNEL, 0);
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

  // ledc (PWM) frequency and resolution setup
  ledcSetup(RED_LED_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
  ledcSetup(GREEN_LED_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
  ledcSetup(BLUE_LED_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);

  // ledc (PWM) pin setup
  ledcAttachPin(RED_LED_PIN, RED_LED_CHANNEL);
  ledcAttachPin(GREEN_LED_PIN, GREEN_LED_CHANNEL);
  ledcAttachPin(BLUE_LED_PIN, BLUE_LED_CHANNEL);

  // Init serial for logging
  Serial.begin(115200);

  Wire.begin(INA_SDA, INA_SCL); // SDA, SCL

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (!ina219.begin()) {
    Serial.println("[ERROR] [POWER]: Failed to find INA219 chip.");
    #define NO_INA
  }

  // Create FreeRTOS queue
  samples_queue = xQueueCreate(8, sizeof(float));

  pinMode(CHARGER_LED_PIN, OUTPUT);
  // Turn off charging indicator LED
  digitalWrite(CHARGER_LED_PIN, HIGH);

  #ifndef NO_INA
  xTaskCreatePinnedToCore(battery_checker_task, "Battery Checker", BAT_TASK_STACK, NULL, BAT_TASK_PRI, NULL, 1);
  #endif

  // Create the mic reader task and pin it to the first core (ID=0)
  xTaskCreatePinnedToCore(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL, 0);

  // Create the Leq calculator task and pin it to the second core (ID=1)
  xTaskCreatePinnedToCore(leq_calculator_task, "Leq Calculator", LEQ_TASK_STACK, NULL, LEQ_TASK_PRI, NULL, 1);
  
  vTaskDelay(pdMS_TO_TICKS(1000)); // Safety
}

void loop() {
  // Execution will never reach this point since we're using FreeRTOS
}