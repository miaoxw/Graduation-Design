#include <Wire.h>
#include "ADXL345.h"
#include "counter.h"

using namespace ADXL345;
using Pedometer::judgeFootstep;

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
	Pedometer::init();

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
	Wire.write(0x9);//典型功耗，50Hz
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(INT_MAP);
	Wire.write(0x0);//发送到INT1脚
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.beginTransmission(address);
	Wire.write(INT_ENABLE);
	Wire.write(0x0);//关中断
	Wire.endTransmission();
	Wire.write(POWER_CTL);
	Wire.write(0x8);//测量模式
	Wire.endTransmission();
}

void loop()
{
	int readings[3];
	getReading(readings);

	double acceleration = sqrt(readings[0] * readings[0] + readings[1] * readings[1] + readings[2] * readings[2]);
	if (judgeFootstep(acceleration))
	{
		digitalWrite(5, HIGH);
		delay(50);
		digitalWrite(5, LOW);
	}
}
