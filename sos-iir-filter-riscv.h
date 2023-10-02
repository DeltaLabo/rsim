#ifndef SOS_IIR_FILTER_H
#define SOS_IIR_FILTER_H

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

extern "C" {
  int sos_filter_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w);
} 
__asm__ (
  //
  // RISC-V implementation of IIR Second-Order Section filter 
  // Assumes a0 and b0 coefficients are one (1.0)
  //
  // float* a2 = input;
  // float* a3 = output;
  // int    a4 = len;
  // float* a5 = coeffs;
  // float* a6 = w; 
  // float  a7 = gain;
  //
  ".text                    \n"
  ".align  4                \n"
  ".global sos_filter_f32   \n"
  ".type   sos_filter_f32,@function\n"
  "sos_filter_f32:          \n"
  "  flw     f0, 0(a5)      \n" // float f0 = coeffs.b1;
  "  flw     f1, 4(a5)      \n" // float f1 = coeffs.b2;
  "  flw     f2, 8(a5)      \n" // float f2 = coeffs.a1;
  "  flw     f3, 12(a5)     \n" // float f3 = coeffs.a2;
  "  flw     f4, 0(a6)      \n" // float f4 = w[0];
  "  flw     f5, 4(a6)      \n" // float f5 = w[1];
  "  loop:                  \n"
  "    bnez    a4, 1f       \n" // for (; len>0; len--) { 
  "    j exit               \n"
  "  i:                     \n"
  "    flw     f6, a2       \n" //   float f6 = *input++;
  "    addi    a2, a2, 4    \n" //   post-increment by 4
  "    fmadd.s f6, f2, f4   \n" //   f6 += f2 * f4; // coeffs.a1 * w0
  "    fmadd.s f6, f3, f5   \n" //   f6 += f3 * f5; // coeffs.a2 * w1
  "    fmv.s   f7, f6       \n" //   f7 = f6; // b0 assumed 1.0
  "    fmadd.s f7, f0, f4   \n" //   f7 += f0 * f4; // coeffs.b1 * w0
  "    fmadd.s f7, f1, f5   \n" //   f7 += f1 * f5; // coeffs.b2 * w1 -> result
  "    fsw     f7, a3       \n" //   *output++ = f7;
  "    addi    a3, a3, 4    \n" //   post-increment by 4
  "    fmv.s   f5, f4       \n" //   f5 = f4; // w1 = w0
  "    fmv.s   f4, f6       \n" //   f4 = f6; // w0 = f6
  "    addi    a4, a4, -1   \n" //   update loop counter
  "    bnez    a4, 1b       \n"
  "    j exit               \n"
  "  exit:                  \n" // }
  "  fsw     f4, 0(a6)      \n" // w[0] = f4;
  "  fsw     f5, 4(a6)      \n" // w[1] = f5;
  "  fmvi    a2, 0          \n" // return 0;
  "  ret                    \n"
);

extern "C" {
  float sos_filter_sum_sqr_f32(float *input, float *output, int len, const SOS_Coefficients &coeffs, SOS_Delay_State &w, float gain);
}
__asm__ (
  //
  // RISC-V implementation of IIR Second-Order section filter with applied gain.
  // Assumes a0 and b0 coefficients are one (1.0)
  // Returns sum of squares of filtered samples
  //
  // float* a2 = input;
  // float* a3 = output;
  // int    a4 = len;
  // float* a5 = coeffs;
  // float* a6 = w;
  // float  a7 = gain;
  //
  ".text                    \n"
  ".align  4                \n"
  ".global sos_filter_sum_sqr_f32 \n"
  ".type   sos_filter_sum_sqr_f32,@function \n"
  "sos_filter_sum_sqr_f32:  \n"
  "  flw     f0, 0(a5)      \n" // float f0 = coeffs.b1;
  "  flw     f1, 4(a5)      \n" // float f1 = coeffs.b2;
  "  flw     f2, 8(a5)      \n" // float f2 = coeffs.a1;
  "  flw     f3, 12(a5)     \n" // float f3 = coeffs.a2;
  "  flw     f4, 0(a6)      \n" // float f4 = w[0];
  "  flw     f5, 4(a6)      \n" // float f5 = w[1];
  "  fmv.s   f6, a7         \n" // float f6 = gain;
  "  fli     f10, 0.0       \n" // float sum_sqr = 0;
  "  loop:                  \n"
  "    bnez    a4, 1f       \n" // for (; len>0; len--) {
  "    j exit               \n"
  "  i:                     \n"
  "    flw     f7, a2       \n" //   float f7 = *input++;
  "    addi    a2, a2, 4    \n" //   post-increment by 4
  "    fmadd.s f7, f2, f4   \n" //   f7 += f2 * f4; // coeffs.a1 * w0
  "    fmadd.s f7, f3, f5   \n" //   f7 += f3 * f5; // coeffs.a2 * w1;
  "    fmv.s   f8, f7       \n" //   f8 = f7; // b0 assumed 1.0
  "    fmadd.s f8, f0, f4   \n" //   f8 += f0 * f4; // coeffs.b1 * w0;
  "    fmadd.s f8, f1, f5   \n" //   f8 += f1 * f5; // coeffs.b2 * w1; 
  "    fmul.s  f9, f8, f6   \n" //   f9 = f8 * f6;  // f8 * gain -> result
  "    fsw     f9, a3       \n" //   *output++ = f9;
  "    addi    a3, a3, 4    \n" //   post-increment by 4
  "    fmv.s   f5, f4       \n" //   f5 = f4; // w1 = w0
  "    fmv.s   f4, f7       \n" //   f4 = f7; // w0 = f7;
  "    fmadd.s f10, f9, f9  \n" //   f10 += f9 * f9; // sum_sqr += f9 * f9;
  "    addi    a4, a4, -1   \n" //   update loop counter
  "    bnez    a4, 1b       \n"
  "    j exit               \n"
  "  exit:                  \n" // }
  "  fsw     f4, a6, 0      \n" // w[0] = f4;
  "  fsw     f5, a6, 4      \n" // w[1] = f5;
  "  fmr     a2, f10        \n" // return sum_sqr; 
  "  ret                    \n" // 
);


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

#endif // SOS_IIR_FILTER_H
