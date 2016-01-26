#include "fall.h"
#include "ADXL345.h"

using namespace fall;
using ADXL345::SCALE;

namespace fall
{
	double lastAcceleration = 1.0;
	//系统初始化的时间就大于1500毫秒，此时间满足要求
	unsigned long lastCrossLFTTime = 0, lastCrossUFTTime = 1500;
	unsigned long lastAlarmTime;

	double velocityX = 0.0;
	double velocityY = 0.0;
	double velocityZ = 0.0;
	unsigned long lastIntegrationTime;
	unsigned char stableCount = 0;
}

bool fall::timeDetect(int readingX, int readingY, int readingZ)
{
	unsigned time = millis();

	//剧烈加速度变化产生的报警持续1秒
	if (time - lastAlarmTime <= 1000)
		return true;

	uint16_t acceleration = sqrt((double)(readingX*readingX + readingY*readingY + readingZ*readingZ));
	if (lastAcceleration<LFT&&acceleration>LFT)
		lastCrossLFTTime = time;
	if (lastAcceleration<UFT&&acceleration>UFT)
		lastCrossUFTTime = time;
	lastAcceleration = acceleration;

	if (lastCrossUFTTime - lastCrossLFTTime < TIME_FE&&time - lastCrossUFTTime <= 1000)
		//满足触发条件且间隔不是太远
	{
		lastAlarmTime = time;
		//Serial.println("Time: alarming");
		return true;
	}
	else
	{
		//Serial.println("Time: not alarming");
		return false;
	}
}

bool fall::velocityDetect(int readingX, int readingY, int readingZ)
{
	unsigned long time = millis();
	unsigned long delta = time - lastIntegrationTime;
	if (abs(readingX - NORMAL_X) <= ADXL345::THRESHOLD_0G&&abs(readingY - NORMAL_Y) <= ADXL345::THRESHOLD_0G&&abs(readingZ - NORMAL_Z) <= ADXL345::THRESHOLD_0G)
		//只受重力时假定一定处于静止状态
	{
		if (++stableCount > 25)
		{
			stableCount = 0;
			velocityX = velocityY = velocityZ = 0.0;
		}
	}
	else
	{
		stableCount = 0;
		if (abs(readingX - NORMAL_X) > ADXL345::THRESHOLD_0G)
			velocityX += (readingX - NORMAL_X) / SCALE * G * delta / 1000;
		if (abs(readingY - NORMAL_Y) > ADXL345::THRESHOLD_0G)
			velocityY += (readingY - NORMAL_Y) / SCALE * G * delta / 1000;
		if (abs(readingZ - NORMAL_Z) > ADXL345::THRESHOLD_0G)
			velocityZ += (readingZ - NORMAL_Z) / SCALE * G * delta / 1000;
	}

	double totalSpeed = sqrt(velocityX*velocityX + velocityY*velocityY + velocityZ*velocityZ);
	lastIntegrationTime = time;
	//Serial.print(readingX);
	//Serial.print(" ");
	//Serial.print(readingY);
	//Serial.print(" ");
	//Serial.println(readingZ);
	//Serial.println(totalSpeed > VELOCITY_LFT ? "Alarming" : "Idling");
	return totalSpeed > VELOCITY_LFT;
}

bool fall::angleDetect(int readingX, int readingY, int readingZ)
{
	//标准重力场可以定义为(0,0,1)，也可以定义为(0,0,64)，总之xy维度一定为0；因此，计算内积时，取Z轴读书即可。
	double dotProduct = readingZ*(double)NORMAL_Z;
	double readingModule = sqrt(readingX*readingX + readingY*readingY + readingZ * readingZ);
	double referenceModule = (double)NORMAL_Z;
	double theta = acos(dotProduct / readingModule / referenceModule) * 180 / PI;
	//Serial.println(int(theta));
	return theta > ANGLE_THRESHOLD;
}
