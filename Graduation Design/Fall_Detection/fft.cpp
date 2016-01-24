#include "fft.h"

using fft::Complex;

fft::Complex::Complex(double real, double image):real(real),image(image)
{
}

Complex fft::Complex::operator +(const Complex another) const
{
	return Complex(real+another.real,image+another.image);
}

Complex fft::Complex::operator -(const Complex another) const
{
	return Complex(real - another.real, image - another.image);
}

Complex fft::Complex::operator *(const Complex another) const
{
	return Complex(real*real - another.image*another.image, real*another.image + image*another.real);
}

Complex fft::Complex::operator /(const Complex another) const
{
	return Complex((real*another.real+image*another.image)/(another.real*another.real+another.image*another.image),(image*another.real-real*another.image) / (another.real*another.real + another.image*another.image));
}

double fft::Complex::module()
{
	return sqrt(real*real + image*image);
}

Complex& fft::Complex::conjugate()
{
	image *= -1.0;
	return *this;
}

void change(Complex f[], int len)
{
	int i, j;
	for (i = 1, j = len / 2; i < len - 1; i++)
	{
		if (i < j)
		{
			Complex temp = f[i];
			f[i] = f[j];
			f[j] = temp;
		}

		//求下一个倒序位
		int k = len / 2;
		//j>=k时，最高位为1
		while (j >= k)
		{
			//最高位改为0
			j -= k;
			//依次逐个比较，直到找到0位
			k /= 2;
		}
		//0位改为1
		j += k;
	}
}

void fft::fft(Complex f[], int len)
{
	change(f, len);
	for (int h = 2; h <= len; h <<= 1)
	{
		Complex wn(cos(-2 * PI / h), sin(-2 * PI / h));
		for (int j = 0; j < len; j += h)
		{
			Complex w(1.0);
			for (int k = j; k < j + h / 2; k++)
			{
				Complex u = f[k];
				Complex t = w*f[k + h / 2];
				f[k] = u + t;
				f[k + h / 2] = u - t;
				w = w*wn;
			}
		}
	}
}

void fft::ifft(Complex f[], int len)
{
	change(f, len);
	for (int h = 2; h <= len; h <<= 1)
	{
		Complex wn(cos(2 * PI / h), sin(2 * PI / h));
		for (int j = 0; j < len; j += h)
		{
			Complex w(1.0);
			for (int k = j; k < j + h / 2; k++)
			{
				Complex u = f[k];
				Complex t = w*f[k + h / 2];
				f[k] = u + t;
				f[k + h / 2] = u - t;
				w = w*wn;
			}
		}
	}

	for (int i = 0; i < len; i++)
		f[i].real /= len;
}
