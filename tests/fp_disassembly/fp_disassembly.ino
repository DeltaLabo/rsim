// This is a barebones copy of Ivan Kostoski's ESP32 Second-Order Sections IIR Filter implementation
// intended only for disassembly

#include <stdint.h>

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

int sos_filter_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w)
{
  //
  // C implementation of IIR Second-Order Section filter 
  // Assumes a0 and b0 coefficients are one (1.0)
  //

  float f0 = coeffs.b1;
  float f1 = coeffs.b2;
  float f2 = coeffs.a1;
  float f3 = coeffs.a2;
  float f4 = w.w0;
  float f5 = w.w1;
  float f6, f7;

  for (; len>0; len--) { 
    f6 = *input++;
    f6 += f2 * f4; // coeffs.a1 * w0
    f6 += f3 * f5; // coeffs.a2 * w1
    f7 = f6; // b0 assumed 1.0
    f7 += f0 * f4; // coeffs.b1 * w0
    f7 += f1 * f5; // coeffs.b2 * w1 -> result
    *output++ = f7;
    f5 = f4; // w1 = w0
    f4 = f6; // w0 = f6
  }
  w.w0 = f4;
  w.w1 = f5;
  return 0;
};

float sos_filter_sum_sqr_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w, float gain)
{
  // ESP32 implementation of IIR Second-Order section filter with applied gain.
  // Assumes a0 and b0 coefficients are one (1.0)
  // Returns sum of squares of filtered samples

  // float* a2 = input;
  // float* a3 = output;
  // int    a4 = len;
  // float* a5 = coeffs;
  // float* a6 = w;
  // float  a7 = gain;

  float f0 = coeffs.b1;
  float f1 = coeffs.b2;
  float f2 = coeffs.a1;
  float f3 = coeffs.a2;
  float f4 = w.w0;
  float f5 = w.w1;
  float f6 = gain;
  float sum_sqr = 0;
  float f7, f8, f9;

  for (; len>0; len--) {
    f7 = *input++;
    f7 += f2 * f4; // coeffs.a1 * w0
    f7 += f3 * f5; // coeffs.a2 * w1;
    f8 = f7; // b0 assumed 1.0
    f8 += f0 * f4; // coeffs.b1 * w0;
    f8 += f1 * f5; // coeffs.b2 * w1; 
    f9 = f8 * f6;  // f8 * gain -> result
    *output++ = f9;
    f5 = f4; // w1 = w0
    f4 = f7; // w0 = f7;
    sum_sqr += f9 * f9; // sum_sqr += f9 * f9;
  }
  w.w0 = f4;
  w.w1 = f5;
  return sum_sqr;
};


/**
 * Envelops above asm functions into C++ class
 */
struct SOS_IIR_Filter {

  const int num_sos;
  const float gain;
  SOS_Coefficients* sos = NULL;
  SOS_Delay_State* w = NULL;

  // Dynamic constructor
  SOS_IIR_Filter(size_t num_sos, const float gain, const SOS_Coefficients _sos[] = NULL): num_sos(num_sos), gain(gain) {
    if (num_sos > 0) {
      sos = new SOS_Coefficients[num_sos];
      if ((sos != NULL) && (_sos != NULL)) memcpy(sos, _sos, num_sos * sizeof(SOS_Coefficients));
      w = new SOS_Delay_State[num_sos]();
    }
  };

  // Template constructor for const filter declaration
  template <size_t Array_Size>
  SOS_IIR_Filter(const float gain, const SOS_Coefficients (&sos)[Array_Size]): SOS_IIR_Filter(Array_Size, gain, sos) {};

  /** 
   * Apply defined IIR Filter to input array of floats, write filtered values to output, 
   * and return sum of squares of all filtered values 
   */
  inline float filter(float* input, float* output, size_t len) {
    if ((num_sos < 1) || (sos == NULL) || (w == NULL)) return 0;
    float* source = input; 
    // Apply all but last Second-Order-Section 
    for(int i=0; i<(num_sos-1); i++) {                
      sos_filter_f32(source, output, len, sos[i], w[i]);      
      source = output;
    }      
    // Apply last SOS with gain and return the sum of squares of all samples  
    return sos_filter_sum_sqr_f32(source, output, len, sos[num_sos-1], w[num_sos-1], gain);
  }

  ~SOS_IIR_Filter() {
    if (w != NULL) delete[] w;
    if (sos != NULL) delete[] sos;
  }

};

//
// For testing only
//
struct No_IIR_Filter {  
  const int num_sos = 0;
  const float gain = 1.0;

  No_IIR_Filter() {};

  inline float filter(float* input, float* output, size_t len) {
    float sum_sqr = 0;
    float s;
    for(int i=0; i<len; i++) {
      s = input[i];
      sum_sqr += s * s;
    }
    if (input != output) {
      for(int i=0; i<len; i++) output[i] = input[i];
    }
    return sum_sqr;
  };
  
};

No_IIR_Filter None;

void setup(){}
void loop(){}
