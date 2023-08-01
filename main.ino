// Source: https://github.com/0015/ThatProject/tree/master/ESP32_MICROPHONE

#include <driver/i2s.h>

// Minimum noise level that the mic can output
#define MIN_NOISE_LEVEL_INT -8388608 // As an int
#define MIN_NOISE_LEVEL_DB 33 // In decibels

// Maximum noise level that the mic can output
#define MAX_NOISE_LEVEL_INT 8388607 // As an int
#define MAX_NOISE_LEVEL_DB 120 // In decibels

// THe outputs are 24*bit, 2's complement integers

// Pins for the I2S interface
#define I2S_WS D3
#define I2S_SD D9
#define I2S_SCK D7

#define I2S_PORT I2S_NUM_0

#define bufferLen 64
//int16_t sBuffer[bufferLen];
int16_t sBuffer[bufferLen];

// Number of bytes in a single noise level measurement
#define wordLen 2

bool outputIndB = false;

void i2s_install(){
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin(){
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);

  delay(1000);
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  delay(500);
}

void loop() {

  /*
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
  if (result == ESP_OK)
  {
    int samples_read = bytesIn / 8;
    if (samples_read > 0) {
      float mean = 0;
      for (int i = 0; i < samples_read; ++i) {
        mean += (sBuffer[i]);
      }
      mean /= samples_read;

      if (outputIndB) {
        // Map the output to decibels
        mean = map(mean, MIN_NOISE_LEVEL_INT, MAX_NOISE_LEVEL_INT, MIN_NOISE_LEVEL_DB, MAX_NOISE_LEVEL_DB);
      }
      // Output the noise level as a 24*bit, 2's complement integer
      else Serial.println(mean);
    }
  }
  */

  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, wordLen, &bytesIn, portMAX_DELAY);
  if (result == ESP_OK)
  {
    if (bytesIn > 0) {
      if (outputIndB) {
        // Map the output to decibels
        sBuffer[0] = map(sBuffer[0], MIN_NOISE_LEVEL_INT, MAX_NOISE_LEVEL_INT, MIN_NOISE_LEVEL_DB, MAX_NOISE_LEVEL_DB);
      }
      // Output the noise level as a 24*bit, 2's complement integer
      else Serial.println(sBuffer[0], BIN);
    }
  }
}