// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef SLM_H
#define SLM_H

#include <math.h>
#include "driver/i2s.h"
#include <Arduino.h>

#include "pins.h"
#include "log-data.h"
#include "color-control.h"
#include "sos-iir-filter-xtensa.h"

// I2S peripheral to use (0 or 1)
#define I2S_PORT          I2S_NUM_0

// SLM Configuration
#define LEQ_PERIOD 2.0 // second(s)
#define LOGGING_PERIOD 5.0 // second(s)
#define MIC_OFFSET_DB 1.3 // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

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
// Sampling
//
#define SAMPLE_RATE       48000 // Hz, fixed to design of IIR filters
#define SAMPLE_BITS       32    // bits
#define SAMPLE_T          int32_t // Sample data type, 32 bits to accommodate the incoming 24-bit samples
#define SAMPLES_SHORT     (SAMPLE_RATE / 8) // ~125ms
#define SAMPLES_LEQ       (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE     (SAMPLES_SHORT / 16)
#define DMA_BANKS         32

void mic_i2s_init();
void mic_i2s_reader_task(void* parameter);
void leq_calculator_task(void* parameter);

#endif // SLM_H