#include "slm.h"


extern bool USE_LOGGING;

extern QueueHandle_t samples_queue;
extern QueueHandle_t logging_queue;

// Current color being displayed on the LEDs
// Initialized to a null value
int currentColor = -1;

// Static buffer for block of samples
float samples[SAMPLES_SHORT] __attribute__((aligned(4)));


extern "C" {
    __asm__ (
        ".text                    \n"
        ".align  4                \n"
        ".global sos_filter_f32   \n"
        ".type   sos_filter_f32,@function\n"
        "sos_filter_f32:          \n"
        "  entry   a1, 16         \n"
        "  lsi     f0, a5, 0      \n"
        "  lsi     f1, a5, 4      \n"
        "  lsi     f2, a5, 8      \n"
        "  lsi     f3, a5, 12     \n"
        "  lsi     f4, a6, 0      \n"
        "  lsi     f5, a6, 4      \n"
        "  loopnez a4, 1f         \n"
        "    lsip    f6, a2, 4    \n"
        "    madd.s  f6, f2, f4   \n"
        "    madd.s  f6, f3, f5   \n"
        "    mov.s   f7, f6       \n"
        "    madd.s  f7, f0, f4   \n"
        "    madd.s  f7, f1, f5   \n"
        "    ssip    f7, a3, 4    \n"
        "    mov.s   f5, f4       \n"
        "    mov.s   f4, f6       \n"
        "  1:                     \n"
        "  ssi     f4, a6, 0      \n"
        "  ssi     f5, a6, 4      \n"
        "  movi.n   a2, 0         \n"
        "  retw.n                 \n"
    );

    __asm__ (
        ".text                    \n"
        ".align  4                \n"
        ".global sos_filter_sum_sqr_f32 \n"
        ".type   sos_filter_sum_sqr_f32,@function \n"
        "sos_filter_sum_sqr_f32:  \n"
        "  entry   a1, 16         \n"
        "  lsi     f0, a5, 0      \n"
        "  lsi     f1, a5, 4      \n"
        "  lsi     f2, a5, 8      \n"
        "  lsi     f3, a5, 12     \n"
        "  lsi     f4, a6, 0      \n"
        "  lsi     f5, a6, 4      \n"
        "  wfr     f6, a7         \n"
        "  const.s f10, 0         \n"
        "  loopnez a4, 1f         \n"
        "    lsip    f7, a2, 4    \n"
        "    madd.s  f7, f2, f4   \n"
        "    madd.s  f7, f3, f5   \n"
        "    mov.s   f8, f7       \n"
        "    madd.s  f8, f0, f4   \n"
        "    madd.s  f8, f1, f5   \n"
        "    mul.s   f9, f8, f6   \n"
        "    ssip    f9, a3, 4    \n"
        "    mov.s   f5, f4       \n"
        "    mov.s   f4, f7       \n"
        "    madd.s  f10, f9, f9  \n"
        "  1:                     \n"
        "  ssi     f4, a6, 0      \n"
        "  ssi     f5, a6, 4      \n"
        "  rfr     a2, f10        \n"
        "  retw.n                 \n"
    );
}


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


// I2S Microphone sampling setup 
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

    // Apply DC blocker,
    // writes filtered samples back to the same buffer.
    float sum_sqr_weighted = DC_BLOCKER.filter(samples, samples, SAMPLES_SHORT);

    // Apply equalization and calculate Z-weighted sum of squares
    sum_sqr_weighted = INMP441.filter(samples, samples, SAMPLES_SHORT);

    // Apply weighting and calculate weigthed sum of squares
    sum_sqr_weighted = A_weighting.filter(samples, samples, SAMPLES_SHORT);

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

  // Data container for logging operations
  LogData logdata;

  Serial.print("[INFO] [SLM]: Mic offset (dBA): ");
  Serial.print(MIC_OFFSET_DB);
  Serial.println(".");

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

      currentColor = leqToColor(Leq_dB);
      currentColor = updateColorArray(currentColor);
      setLEDColor(currentColor);

      Serial.print("[INFO] [SLM]: Local reading (dBA): ");
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

        if (USE_LOGGING) {
          logdata = {String(Logging_leq), eq_feed_key};
          xQueueSend(logging_queue, &logdata, portMAX_DELAY);

          logdata = {String(Max_leq), max_feed_key};
          xQueueSend(logging_queue, &logdata, portMAX_DELAY);

          logdata = {String(Min_leq), min_feed_key};
          xQueueSend(logging_queue, &logdata, portMAX_DELAY);
        }

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