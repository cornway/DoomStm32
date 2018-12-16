#include "fft.h"

#define SWAP(type, a, b) do {type c = b; b = a; a = c;} while (0)
extern double PI_CONST;

int rev (int num, int lg_n)
{
    int res = 0;
	for (int i=0; i<lg_n; ++i)
		if (num & (1<<i))
			res |= 1<<(lg_n-1-i);
	return res;
}

static BaseComplex base_mul (BaseComplex base1, BaseComplex base2)
{
    base1.real = base1.real * base2.real - base1.imag * base2.imag;
    base1.imag = base1.real * base2.imag + base1.imag * base2.real;
    return base1;
}

void fft (BaseComplex *a, int size, bool invert)
{
    int n = size;
	int lg_n = 0;
	while ((1 << lg_n) < n)  ++lg_n;
 
	for (int i=0; i<n; ++i)
		if (i < rev(i,lg_n))
			SWAP(BaseComplex, a[i], a[rev(i,lg_n)]);
 
	for (int len=2; len<=n; len<<=1) {
		double ang = 2*PI_CONST/len * (invert ? -1 : 1);
		BaseComplex wlen = {cos(ang), sin(ang)};
		for (int i=0; i<n; i+=len) {
			BaseComplex w = {1.0, 0.0};
			for (int j=0; j<len/2; ++j) {
				BaseComplex u = a[i+j],  v = base_mul(a[i+j+len/2], w);
				a[i+j].real = u.real + v.real;
                a[i+j].imag = u.imag + v.imag;
                a[i+j+len/2].real = u.real - v.real;
                a[i+j+len/2].imag = u.imag - v.imag;
                w = base_mul(w, wlen);
			}
		}
	}
	if (invert)
		for (int i=0; i<n; ++i) {
			a[i].real /= n;
            a[i].imag /= n;
        }
}
