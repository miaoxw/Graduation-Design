#include <Wire.h>
#include "ADXL345.h"
#include "counter.h"
#include <LTask.h>
#include "stddef.h"
#include "threadStarter.h"
#include "priority.h"
#include <string.h>
#include "fall.h"

using namespace ADXL345;
using Pedometer::judgeFootstep;
using fall::judgeFall;

VM_SIGNAL_ID fallAlarm, sendMessage;
vm_thread_mutex_struct mutexSensorDataWrite,mutexReaderCount,mutexSensor;
int readCount;
int globalReadings[3];
double acceleration;

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
	pinMode(13, OUTPUT);
	Serial.begin(57600);
	Pedometer::init();

	Wire.begin();
	delay(50);

	//Reset offset
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

	//TODO: ADXL345有关静止唤醒的中断

	Wire.beginTransmission(address);
	Wire.write(INT_ENABLE);
	Wire.write(0x0);//关中断
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(POWER_CTL);
	Wire.write(0x8);//测量模式
	Wire.endTransmission();

	//Adjustment
	int x = 0, y = 0, z = 0;
	for (int i = 1; i <= 20; i++)
	{
		Wire.beginTransmission(address);
		Wire.write(X_low);
		Wire.endTransmission();
		Wire.requestFrom(address, 6);
		while (Wire.available() < 6)
			;
		x += (int16_t)(Wire.read() | Wire.read() << 8);
		y += (int16_t)(Wire.read() | Wire.read() << 8);
		z += (int16_t)(Wire.read() | Wire.read() << 8);
		delay(15);
	}
	x /= 20;
	y /= 20;
	z /= 20;
	int8_t xOffset = -round(x / 4.0);
	int8_t yOffset = -round(y / 4.0);
	int8_t zOffset = -round((z - 64) / 4.0);

	//Write offset
	Wire.beginTransmission(address);
	Wire.write(OFSX);
	Wire.write(xOffset);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSY);
	Wire.write(yOffset);
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(OFSZ);
	Wire.write(zOffset);
	Wire.endTransmission();

	//初始化信号量等
	readCount = 0;
	vm_mutex_create(&mutexReaderCount);
	vm_mutex_create(&mutexSensorDataWrite);
	vm_mutex_create(&mutexSensor);
	fallAlarm = vm_signal_init();
	sendMessage = vm_signal_init();
	
	//建立其它工作线程
	ThreadStarter startBlock;

	startBlock.func = fallDetect;
	startBlock.priority = Prority::FALL_DETECTOR;
	LTask.remoteCall(createThread, &startBlock);

	startBlock.func = pedometer;
	startBlock.priority = Prority::PEDOMETER;
	LTask.remoteCall(createThread, &startBlock);

	startBlock.func = dataCollector;
	startBlock.priority = Prority::DATA_COLLECTOR;
	LTask.remoteCall(createThread, &startBlock);
}

boolean createThread(void *ptr)
{
	VM_THREAD_FUNC func = ((ThreadStarter *)ptr)->func;
	VMUINT8 priority = ((ThreadStarter *)ptr)->priority;

	vm_thread_create(func, NULL, priority);

	return true;
}

VMINT32 fallDetect(VM_THREAD_HANDLE thread_handle, void *userData)
{
	int readings[3];
	double acceleration;

	delay(20000);
	uint32_t loopStart = millis();

	while (true)
	{
		vm_mutex_lock(&mutexSensor);
		vm_mutex_lock(&mutexReaderCount);
		readCount++;
		if (readCount == 1)
			vm_mutex_lock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);
		vm_mutex_unlock(&mutexSensor);

		//数据迁移至本地
		memcpy(readings, globalReadings, sizeof(globalReadings));
		acceleration = ::acceleration;

		vm_mutex_unlock(&mutexReaderCount);
		readCount--;
		if (readCount == 0)
			vm_mutex_unlock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);

		if (judgeFall(readings[0], readings[1], readings[2]))
		{
			Serial.println("Falling!");
			vm_signal_post(fallAlarm);
		}

		uint32_t now = millis();
		uint32_t toDelay = (now - loopStart) % 20;
		if (!toDelay)
			toDelay = 20;
		loopStart += 20;
		delay(toDelay);
	}
}

VMINT32 pedometer(VM_THREAD_HANDLE thread_handle, void *userData)
{
	int readings[3];
	double acceleration;

	long stepCount = 0;

	delay(20000);
	uint32_t loopStart = millis();

	while (true)
	{
		vm_mutex_lock(&mutexSensor);
		vm_mutex_lock(&mutexReaderCount);
		readCount++;
		if (readCount == 1)
			vm_mutex_lock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);
		vm_mutex_unlock(&mutexSensor);

		//数据迁移至本地
		memcpy(readings, globalReadings, sizeof(globalReadings));
		acceleration = ::acceleration;

		vm_mutex_unlock(&mutexReaderCount);
		readCount--;
		if (readCount == 0)
			vm_mutex_unlock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);

		if (judgeFootstep(acceleration))
		{
			stepCount++;
			Serial.println("A step!");
		}

		uint32_t now = millis();
		uint32_t toDelay = (now - loopStart) % 20;
		if (!toDelay)
			toDelay = 20;
		loopStart += 20;
		delay(toDelay);
	}
}

VMINT32 dataCollector(VM_THREAD_HANDLE thread_handle, void *userData)
{
	uint32_t loopStart = millis();

	uint32_t signal = HIGH;

	while (true)
	{
		int readings[3];
		getReading(readings);

		vm_mutex_lock(&mutexSensor);
		vm_mutex_lock(&mutexSensorDataWrite);
		memcpy(globalReadings, readings, sizeof(readings));
		acceleration = sqrt(readings[0] * readings[0] / 4096.0 + readings[1] * readings[1] / 4096.0 + readings[2] * readings[2] / 4096.0);
		digitalWrite(13, signal);
		signal = 1 - signal;
		vm_mutex_unlock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexSensor);

		uint32_t now = millis();
		uint32_t toDelay = (now - loopStart) % 20;
		if (!toDelay)
			toDelay = 20;
		loopStart += 20;
		delay(toDelay);
	}
}

void loop()
{
	delay(1000);
}
