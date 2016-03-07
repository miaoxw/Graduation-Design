#include <Wire\Wire.h>
#include "ADXL345.h"

using namespace ADXL345;

String str = "";

void getReading(int *store)
{
	int x, y, z;

	Wire.beginTransmission(address);
	Wire.write(X_low);
	Wire.endTransmission();
	Wire.requestFrom(address, 6);
	while (Wire.available() < 6)
		;

	x = (int16_t)(Wire.read() | Wire.read() << 8);
	y = (int16_t)(Wire.read() | Wire.read() << 8);
	z = (int16_t)(Wire.read() | Wire.read() << 8);
	store[0] = x;
	store[1] = y;
	store[2] = z;
}

void setup()
{
	Serial.begin(57600);
	//Wire.begin();
	delay(50);
	pinMode(4, INPUT);

	while (digitalRead(4) == LOW)
		;

	////Reset offset
	//Wire.beginTransmission(address);
	//Wire.write(OFSX);
	//Wire.write(0);
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(OFSY);
	//Wire.write(0);
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(OFSZ);
	//Wire.write(0);
	//Wire.endTransmission();

	//Wire.beginTransmission(address);
	//Wire.write(DATA_FORMAT);
	//Wire.write(0x2);//+-8g, 10-bit resolution
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(BW_RATE);
	//Wire.write(0x9);//典型功耗，50Hz
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(INT_MAP);
	//Wire.write(0x0);//发送到INT1脚
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(INT_ENABLE);
	//Wire.write(0x0);//关中断
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(POWER_CTL);
	//Wire.write(0x8);//测量模式
	//Wire.endTransmission();

	////Adjustment
	//int x = 0, y = 0, z = 0;
	//for (int i = 1; i <= 20; i++)
	//{
	//	Wire.beginTransmission(address);
	//	Wire.write(X_low);
	//	Wire.endTransmission();
	//	Wire.requestFrom(address, 6);
	//	while (Wire.available() < 6)
	//		;
	//	x += (int16_t)(Wire.read() | Wire.read() << 8);
	//	y += (int16_t)(Wire.read() | Wire.read() << 8);
	//	z += (int16_t)(Wire.read() | Wire.read() << 8);
	//	delay(15);
	//}
	//x /= 20;
	//y /= 20;
	//z /= 20;
	//int8_t xOffset = -round(x / 4.0);
	//int8_t yOffset = -round(y / 4.0);
	//int8_t zOffset = -round((z - 64) / 4.0);

	////Write offset
	//Wire.beginTransmission(address);
	//Wire.write(OFSX);
	//Wire.write(xOffset);
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(OFSY);
	//Wire.write(yOffset);
	//Wire.endTransmission();
	//Wire.beginTransmission(address);
	//Wire.write(OFSZ);
	//Wire.write(zOffset);
	//Wire.endTransmission();

	////设置中断
	//attachInterrupt(digitalPinToInterrupt(2), dataPulse, RISING);

	////开中断
	//Wire.beginTransmission(address);
	//Wire.write(INT_ENABLE);
	//Wire.write(0x80);
	//Wire.endTransmission();
}

void dataPulse()
{

}

void loop()
{
	double floatNumber = 1.2345667888654;
	str += floatNumber*10000;
	str += "\n";
	if (str.length() > 128)
	{
		Serial.print(str);
		str = "";
	}
	delay(15);
}
