// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef SLM_PARAMS_H
#define SLM_PARAMS_H

#include <math.h>
#include <cstddef>
#include <cstring>
#include "driver/i2s.h"

#include "pins.h"
#include "color-control.h"
#include "log-data.h"
#include "adafruit-io.h"


// SLM Configuration
#define LEQ_PERIOD 2.0 // second(s)
#define LOGGING_PERIOD 20.0 // second(s)
#define MIC_OFFSET_DB 1.3 // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

#define LOGGING_QUEUE_SIZE 25

// I2S peripheral to use (0 or 1)
#define I2S_PORT          I2S_NUM_0

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


struct SOS_Coefficients {
    float b1;
    float b2;
    float a1;
    float a2;
};

struct SOS_Delay_State {
    float w0 = 0;
    float w1 = 0;
};

extern "C" {
    int sos_filter_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w);
    float sos_filter_sum_sqr_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w, float gain);
}

struct SOS_IIR_Filter {
    const int num_sos;
    const float gain;
    SOS_Coefficients* sos = NULL;
    SOS_Delay_State* w = NULL;

    SOS_IIR_Filter(size_t num_sos, const float gain, const SOS_Coefficients _sos[] = NULL)
        : num_sos(num_sos), gain(gain) {
        if (num_sos > 0) {
            sos = new SOS_Coefficients[num_sos];
            if ((sos != NULL) && (_sos != NULL)) memcpy(sos, _sos, num_sos * sizeof(SOS_Coefficients));
            w = new SOS_Delay_State[num_sos]();
        }
    }

    template <size_t Array_Size>
    SOS_IIR_Filter(const float gain, const SOS_Coefficients (&sos)[Array_Size])
        : SOS_IIR_Filter(Array_Size, gain, sos) {}

    inline float filter(float* input, float* output, size_t len) {
        if ((num_sos < 1) || (sos == NULL) || (w == NULL)) return 0;
        float* source = input;
        for (int i = 0; i < (num_sos - 1); i++) {
            sos_filter_f32(source, output, len, sos[i], w[i]);
            source = output;
        }
        return sos_filter_sum_sqr_f32(source, output, len, sos[num_sos - 1], w[num_sos - 1], gain);
    }

    ~SOS_IIR_Filter() {
        if (w != NULL) delete[] w;
        if (sos != NULL) delete[] sos;
    }
};

#endif // SLM_PARAMS_H