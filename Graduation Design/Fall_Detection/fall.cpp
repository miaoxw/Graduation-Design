#include "fall.h"
#include "ADXL345.h"

using namespace fall;
using ADXL345::SCALE;

namespace fall
{
	double lastAcceleration = 1.0;
	//ϵͳ��ʼ����ʱ��ʹ���1500���룬��ʱ������Ҫ��
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

	//���Ҽ��ٶȱ仯�����ı�������1��
	if (time - lastAlarmTime <= 1000)
		return true;

	uint16_t acceleration = sqrt((double)(readingX*readingX + readingY*readingY + readingZ*readingZ));
	if (lastAcceleration<LFT&&acceleration>LFT)
		lastCrossLFTTime = time;
	if (lastAcceleration<UFT&&acceleration>UFT)
		lastCrossUFTTime = time;
	lastAcceleration = acceleration;

	if (lastCrossUFTTime - lastCrossLFTTime < TIME_FE&&time - lastCrossUFTTime <= 1000)
		//���㴥�������Ҽ������̫Զ
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
		//ֻ������ʱ�ٶ�һ�����ھ�ֹ״̬
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
	//��׼���������Զ���Ϊ(0,0,1)��Ҳ���Զ���Ϊ(0,0,64)����֮xyά��һ��Ϊ0����ˣ������ڻ�ʱ��ȡZ����鼴�ɡ�
	double dotProduct = readingZ*(double)NORMAL_Z;
	double readingModule = sqrt(readingX*readingX + readingY*readingY + readingZ * readingZ);
	double referenceModule = (double)NORMAL_Z;
	double theta = acos(dotProduct / readingModule / referenceModule) * 180 / PI;
	//Serial.println(int(theta));
	return theta > ANGLE_THRESHOLD;
}
