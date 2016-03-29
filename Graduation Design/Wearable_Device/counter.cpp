#include "counter.h"

using namespace Pedometer;

namespace Pedometer
{
	double accelerationRecord[150];
	double lastAcceleration;
	double threshold;
	double accelerationHistory[16];
	uint8_t count;
	unsigned long lastPeakTick;
	unsigned long tick;

	const double PEDOMETER_THRESHOLD_LOWER_BOUND=1.02;
	const double PEDOMETER_GRAVITY_BASIS=1.0;
}

void Pedometer::init()
{
	threshold=PEDOMETER_THRESHOLD_LOWER_BOUND;
	count=0;
	tick=0;
	lastAcceleration=1.0;
	lastPeakTick=0;

	for(int i=0; i<16; i++)
		accelerationHistory[i]=1.0;
}

bool Pedometer::judgeFootstep(double acceleration)
{
	tick++;

	if(isnan(acceleration))
		return false;

	bool ret=false;
	static bool rising;

	//均值滤波，窗口大小已经扩充至16
	for(int i=0; i<15; i++)
		accelerationHistory[i+1]=accelerationHistory[i];
	accelerationHistory[0]=acceleration;

	double filteredAcceleration=0.0;
	for(int i=0; i<16; i++)
		filteredAcceleration+=accelerationHistory[i];
	filteredAcceleration/=16;

	//此处存入滤波后的数据
	accelerationRecord[count++]=filteredAcceleration;

	if(count==150u)
	{
		count=0;

		double maxAcceleration=-1e4;
		for(int i=0; i<150; i++)
		{
			if(accelerationRecord[i]>maxAcceleration)
				maxAcceleration=accelerationRecord[i];
		}

		threshold=(maxAcceleration+PEDOMETER_GRAVITY_BASIS)/2;
		if(threshold<PEDOMETER_THRESHOLD_LOWER_BOUND)
			threshold=PEDOMETER_THRESHOLD_LOWER_BOUND;
	}

	if(filteredAcceleration>=lastAcceleration)
		rising=true;
	else
	{
		if(lastAcceleration>=threshold&&rising)
		{
			unsigned long currentTick=tick;
			unsigned long deltaTick=currentTick-lastPeakTick;

			if(deltaTick>FOOTSTEP_TIME_LOWER_BOUND&&deltaTick<FOOTSTEP_TIME_UPPER_BOUND)
				ret=true;

			lastPeakTick=currentTick;
		}
		rising=false;
	}

	lastAcceleration=filteredAcceleration;
	return ret;
}
