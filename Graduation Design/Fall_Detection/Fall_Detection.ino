#include <Wire.h>

namespace ADXL345
{
	const uint8_t address = 0x53;
	const uint8_t OFSX = 0x1E;
	const uint8_t OFSY = 0x1F;
	const uint8_t OFSZ = 0x20;
	const uint8_t BW_RATE = 0x2C;
	const uint8_t POWER_CTL = 0x2D;
	const uint8_t INT_ENABLE = 0x2E;
	const uint8_t INT_MAP = 0x2F;
	const uint8_t INT_SOURCE = 0x30;
	const uint8_t DATA_FORMAT = 0x31;

	const uint8_t X_low = 0x32;
	const uint8_t Y_low = 0x34;
	const uint8_t Z_low = 0x36;

	const uint8_t THRESHOLD = 5u;
};

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

	double lastAcceleration = 1.0;
	//系统初始化的时间就大于1500毫秒，此时间满足要求
	unsigned long lastCrossLFTTime = 0, lastCrossUFTTime = 1500;
	unsigned long lastAlarmTime;

	double velocityX = 0.0;
	double velocityY = 0.0;
	double velocityZ = 0.0;
	unsigned long lastIntegrationTime;
	unsigned char stableCount = 0;

	bool timeDetect(int readingX, int readingY, int readingZ)
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

	bool velocityDetect(int readingX, int readingY, int readingZ)
	{
		unsigned long time = millis();
		unsigned long delta = time - lastIntegrationTime;
		if (abs(readingX - NORMAL_X) <= ADXL345::THRESHOLD&&abs(readingY - NORMAL_Y) <= ADXL345::THRESHOLD&&abs(readingZ - NORMAL_Z) <= ADXL345::THRESHOLD)
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
			if (abs(readingX - NORMAL_X) > ADXL345::THRESHOLD)
				velocityX += (readingX - NORMAL_X) / SCALE * G * delta / 1000;
			if (abs(readingY - NORMAL_Y) > ADXL345::THRESHOLD)
				velocityY += (readingY - NORMAL_Y) / SCALE * G * delta / 1000;
			if (abs(readingZ - NORMAL_Z) > ADXL345::THRESHOLD)
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

	bool angleDetect(int readingX, int readingY, int readingZ)
	{
		//标准重力场可以定义为(0,0,1)，也可以定义为(0,0,64)，总之xy维度一定为0；因此，计算内积时，取Z轴读书即可。
		double dotProduct = readingZ*(double)NORMAL_Z;
		double readingModule = sqrt(readingX*readingX + readingY*readingY + readingZ * readingZ);
		double referenceModule = (double)NORMAL_Z;
		double theta = acos(dotProduct / readingModule / referenceModule) * 180 / PI;
		//Serial.println(int(theta));
		return theta > ANGLE_THRESHOLD;
	}
};

using namespace ADXL345;
using fall::lastIntegrationTime;

void getReading(int *store)
{
	int x, y, z;

	Wire.beginTransmission(address);
	Wire.write(X_low);
	Wire.endTransmission();
	Wire.requestFrom(address, 2);
	while (Wire.available() < 2);
	x = (int16_t)(Wire.read() | Wire.read() << 8);

	Wire.beginTransmission(address);
	Wire.write(Y_low);
	Wire.endTransmission();
	Wire.requestFrom(address, 2);
	while (Wire.available() < 2);
	y = (int16_t)(Wire.read() | Wire.read() << 8);

	Wire.beginTransmission(address);
	Wire.write(Z_low);
	Wire.endTransmission();
	Wire.requestFrom(address, 2);
	while (Wire.available() < 2);
	z = (int16_t)(Wire.read() | Wire.read() << 8);

	store[0] = x;
	store[1] = y;
	store[2] = z;
}

void setup()
{
	Wire.begin();
	delay(50);

	Serial.begin(57600);

	//Write offset
	Wire.beginTransmission(address);
	Wire.write(OFSX);
	Wire.write(0);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSY);
	Wire.write(0);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSZ);
	Wire.write(0);
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(DATA_FORMAT);
	Wire.write(0x2);//+-8g, 10-bit resolution
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(BW_RATE);
	Wire.write(0x7);//典型功耗，12.5Hz
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(INT_MAP);
	Wire.write(0x0);//发送到INT1脚
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(POWER_CTL);
	Wire.write(0x8);//测量模式
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(INT_ENABLE);
	Wire.write(0x0);//关中断
	Wire.endTransmission();

	int x = 0, y = 0, z = 0;
	for (int i = 1; i <= 20; i++)
	{
		Wire.beginTransmission(address);
		Wire.write(X_low);
		Wire.endTransmission();
		Wire.requestFrom(address, 2);
		while (Wire.available() < 2);
		x += (int16_t)(Wire.read() | Wire.read() << 8);

		Wire.beginTransmission(address);
		Wire.write(Y_low);
		Wire.endTransmission();
		Wire.requestFrom(address, 2);
		while (Wire.available() < 2);
		y += (int16_t)(Wire.read() | Wire.read() << 8);

		Wire.beginTransmission(address);
		Wire.write(Z_low);
		Wire.endTransmission();
		Wire.requestFrom(address, 2);
		while (Wire.available() < 2);
		z += (int16_t)(Wire.read() | Wire.read() << 8);
		delay(15);
	}
	x /= 20;
	y /= 20;
	z /= 20;
	int8_t xOff = -round(x / 4.0);
	int8_t yOff = -round(y / 4.0);
	int8_t zOff = -round((z - 64) / 4.0);

	//Write offset
	Wire.beginTransmission(address);
	Wire.write(OFSX);
	Wire.write(xOff);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSY);
	Wire.write(yOff);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSZ);
	Wire.write(zOff);
	Wire.endTransmission();
	delay(100);
	lastIntegrationTime = millis();
}

void loop()
{
	int readings[3];
	getReading(readings);

	bool judge1 = fall::timeDetect(readings[0], readings[1], readings[2]);
	bool judge2 = fall::velocityDetect(readings[0], readings[1], readings[2]);
	bool judge3 = fall::angleDetect(readings[0], readings[1], readings[2]);

	if (judge1&&judge2&&judge3)
		Serial.println("ALARM!");

}
