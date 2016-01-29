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
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
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
	Wire.write(INT_ENABLE);
	Wire.write(0x0);//关中断
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(POWER_CTL);
	Wire.write(0x8);//测量模式
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
}

void loop()
{
	int readings[3];
	getReading(readings);

	double acceleration = sqrt(readings[0] * readings[0] / 4096.0 + readings[1] * readings[1] / 4096.0 + readings[2] * readings[2] / 4096.0);
	bool result=judgeFootstep(acceleration);
	if (result)
	{
		digitalWrite(5, HIGH);
		delay(50);
		digitalWrite(5, LOW);
	}
	Serial.println(acceleration);
	Serial.println(result);
}
