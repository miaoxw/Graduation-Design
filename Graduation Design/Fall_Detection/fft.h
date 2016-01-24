#ifndef _FFT_h
#define _FFT_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
namespace fft
{
	struct Complex
	{
		public:
			double real;
			double image;
			
			Complex(double real=0.0, double image=0.0);
			Complex operator +(const Complex another) const;
			Complex operator -(const Complex another) const;
			Complex operator *(const Complex another) const;
			Complex operator /(const Complex another) const;
			double module();
			Complex& conjugate();
	};
	
	void fft(Complex f[], int N);
	void ifft(Complex f[], int N);
};
#endif

