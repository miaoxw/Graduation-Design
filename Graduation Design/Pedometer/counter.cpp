#include "counter.h"

using namespace Pedometer;

namespace Pedometer
{
	double accelerationRecord[150];
	double lastAcceleration;
	double threshold;
	double accelerationHistory[8];
	uint8_t count;
	unsigned long lastCrossUpTime, lastCrossDownTime;
}

void Pedometer::init()
{
	threshold = 1.35;
	count = 0;
	lastAcceleration = 1.0;
	lastCrossUpTime = lastCrossDownTime = 0;

	for (int i = 0; i < 8; i++)
		accelerationHistory[i] = 1.0;
}

bool Pedometer::judgeFootstep(double acceleration)
{
	bool ret = false;

	accelerationRecord[count++] = acceleration;

	//均值滤波，当前新数据权重增加为2
	for (int i = 0; i < 7; i++)
		accelerationHistory[i + 1] = accelerationHistory[i];
	accelerationHistory[0] = acceleration;
	
	double filteredAcceleration = acceleration;
	for (int i = 0; i < 8; i++)
		filteredAcceleration += accelerationHistory[i];
	filteredAcceleration /= 9.0;
	acceleration = filteredAcceleration;
	
	if (count == 150u)
	{
		count = 0;

		double minAcceleration = 1e4;
		double maxAcceleration = -1e4;
		for (int i = 0; i < 150; i++)
		{
			if (accelerationRecord[i] < minAcceleration)
				minAcceleration = accelerationRecord[i];
			if (accelerationRecord[i] > maxAcceleration)
				maxAcceleration = accelerationRecord[i];
		}

		threshold = (minAcceleration + maxAcceleration) / 2;
	}
	if (lastAcceleration < threshold&&acceleration > threshold)
		lastCrossUpTime = millis();
	if (lastAcceleration > threshold&&acceleration < threshold)
	{
		lastCrossDownTime = millis();
		unsigned long deltaTime = lastCrossDownTime - lastCrossUpTime;
		if (deltaTime >= FOOTSTEP_TIME_LOWER_BOUND&&deltaTime <= FOOTSTEP_TIME_UPPER_BOUND)
			ret = true;
	}

	lastAcceleration = acceleration;
	return ret;
}
