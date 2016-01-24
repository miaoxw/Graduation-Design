#ifndef _FALL_h
#define _FALL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

namespace fall
{
	const double SCALE = 64.0;
	const uint8_t LFT = 41u;
	const uint8_t UFT = 180u;
	const int ANGLE_THRESHOLD = 60;
	const double VELOCITY_LFT = 0.7;
	const unsigned long TIME_FE = 350u;
	const double GRAVITY_ACCELERATION = 1.0;

	const double G = 9.81;

	const int NORMAL_X = 0;
	const int NORMAL_Y = 0;
	const int NORMAL_Z = 64;

	extern unsigned long lastIntegrationTime;

	bool timeDetect(int readingX, int readingY, int readingZ);

	bool velocityDetect(int readingX, int readingY, int readingZ);

	bool angleDetect(int readingX, int readingY, int readingZ);
};

#endif

