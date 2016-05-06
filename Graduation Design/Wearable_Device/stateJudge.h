#ifndef _SPORTSJUDGE_h
#define _SPORTSJUDGE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
namespace SportsJudge
{
	enum SportState
	{
		Sleeping,Awake,Sporting
	};

	SportState getNewState(double acceleration,int accX,int accY,int accZ);
}
#endif

