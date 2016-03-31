#ifndef _SPORTSJUDGE_h
#define _SPORTSJUDGE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
namespace SportsJudge
{
	bool isSporting(double acceleration);
}
#endif

