#ifndef FFT_FAST_H
#define FFT_FAST_H

#include <stdint.h>
#include "math.h"

typedef struct {
    double real;
    double imag;
} BaseComplex;

int rev (int num, int lg_n);

void fft (BaseComplex *a, int size, bool invert);

#endif  /*FFT_FAST_H*/
