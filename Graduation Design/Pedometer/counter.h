#ifndef _COUNTER_h
#define _COUNTER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

namespace Pedometer
{
	const unsigned long FOOTSTEP_TIME_LOWER_BOUND = 125u;
	const unsigned long FOOTSTEP_TIME_UPPER_BOUND = 1000u;

	void init();
	bool judgeFootstep(double acceleration);
}

#endif

