#include <stddef.h>
#include <string.h>

#include <LBTServer.h>
#include <LDateTime.h>
#include <LTask.h>
#include <vmpwr.h>
#include <vmthread.h>
#include <Wire.h>

#include "cJSON.h"
#include "timestamp.h"

#include "ADXL345.h"
#include "command.h"
#include "counter.h"
#include "fall.h"
#include "priority.h"
#include "sportsJudge.h"
#include "StatisticItem.h"
#include "ThreadStarter.h"

using namespace ADXL345;
using namespace std;
using Pedometer::judgeFootstep;
using fall::judgeFall;
using Command::CommandHeap;
using Command::CommandItem;
using SportsJudge::isSporting;
using Statistic::SendMessageQueue;
using Statistic::StatisticType;

VM_SIGNAL_ID fallAlarm,sendMessage,bluetoothOperationPermission;
vm_thread_mutex_struct mutexSensorDataWrite,mutexReaderCount,mutexSensor,mutexCommand,mutexMessage;
int readCount;
volatile int globalReadings[3];
volatile double acceleration;
CommandHeap commandHeap;
SendMessageQueue messageQueue;

void inline getReading(int *store)
{
	int x,y,z;

	Wire.beginTransmission(address);
	Wire.write(X_low);
	Wire.endTransmission();
	Wire.requestFrom(address,6);
	while(Wire.available()<6)
		;

	x=(int16_t)(Wire.read()|Wire.read()<<8);
	y=(int16_t)(Wire.read()|Wire.read()<<8);
	z=(int16_t)(Wire.read()|Wire.read()<<8);
	store[0]=x;
	store[1]=y;
	store[2]=z;
}

void setup()
{
	pinMode(13,OUTPUT);
	Serial.begin(57600);

	if(!LBTServer.begin((uint8_t*)"LinkIt ONE"))
		vm_reboot_normal_start();

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
	int x=0,y=0,z=0;
	for(int i=1;i<=20;i++)
	{
		Wire.beginTransmission(address);
		Wire.write(X_low);
		Wire.endTransmission();
		Wire.requestFrom(address,6);
		while(Wire.available()<6)
			;
		x+=(int16_t)(Wire.read()|Wire.read()<<8);
		y+=(int16_t)(Wire.read()|Wire.read()<<8);
		z+=(int16_t)(Wire.read()|Wire.read()<<8);
		delay(15);
	}
	x/=20;
	y/=20;
	z/=20;
	int8_t xOffset=-round(x/4.0);
	int8_t yOffset=-round(y/4.0);
	int8_t zOffset=-round((z-64)/4.0);

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
	readCount=0;
	vm_mutex_create(&mutexReaderCount);
	vm_mutex_create(&mutexSensorDataWrite);
	vm_mutex_create(&mutexSensor);
	vm_mutex_create(&mutexCommand);
	vm_mutex_create(&mutexMessage);
	fallAlarm=vm_signal_init();
	sendMessage=vm_signal_init();
	bluetoothOperationPermission=vm_signal_init();

	//建立其它工作线程
	ThreadStarter startBlock;
	startBlock.func=dataCollector;
	startBlock.priority=Priority::DATA_COLLECTOR;
	LTask.remoteCall(createThread,&startBlock);

	startBlock.func=dataFetcher;
	startBlock.priority=Priority::DATA_FETCHER;
	LTask.remoteCall(createThread,&startBlock);

	startBlock.func=fallAlarmSender;
	startBlock.priority=Priority::FALL_ALARM;
	LTask.remoteCall(createThread,&startBlock);

	startBlock.func=blueToothConnector;
	startBlock.priority=Priority::BLUETOOTH_CONNECTOR;
	LTask.remoteCall(createThread,&startBlock);

	startBlock.func=blueToothTransmitter;
	startBlock.priority=Priority::COMMAND_TX;
	LTask.remoteCall(createThread,&startBlock);

	startBlock.func=blueToothReceiver;
	startBlock.priority=Priority::COMMAND_RX;
	LTask.remoteCall(createThread,&startBlock);
}

boolean createThread(void *ptr)
{
	VM_THREAD_FUNC func=((ThreadStarter *)ptr)->func;
	VMUINT8 priority=((ThreadStarter *)ptr)->priority;

	vm_thread_create(func,NULL,priority);

	return true;
}

VMINT32 dataFetcher(VM_THREAD_HANDLE thread_handle,void *userData)
{
	int readings[3];
	double acceleration;

	long stepCount=0;
	unsigned int currentStateStart;
	bool sportingNow=false;

	uint32_t loopStart=millis();


	LDateTime.getRtc(&currentStateStart);
	while(true)
	{
		vm_mutex_lock(&mutexSensor);
		vm_mutex_lock(&mutexReaderCount);
		readCount++;
		if(readCount==1)
			vm_mutex_lock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);
		vm_mutex_unlock(&mutexSensor);

		//数据迁移至本地
		memcpy(readings,(const void*)globalReadings,sizeof(globalReadings));
		acceleration=::acceleration;

		digitalWrite(13,HIGH);

		vm_mutex_lock(&mutexReaderCount);
		readCount--;
		if(readCount==0)
			vm_mutex_unlock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexReaderCount);

		if(judgeFall(readings[0],readings[1],readings[2]))
		{
			Serial.println("Falling!");
			vm_signal_post(fallAlarm);
		}

		bool newestState=isSporting(acceleration);
		if(newestState!=sportingNow)
		{
			cJSON *messageToSend=cJSON_CreateObject();

			unsigned int currentTimeStamp;
			LDateTime.getRtc(&currentTimeStamp);

			cJSON *statisticInfo=cJSON_CreateObject();
			cJSON_AddBoolToObject(statisticInfo,"sporting",sportingNow);
			cJSON_AddNumberToObject(statisticInfo,"steps",stepCount);

			StatisticType messageType=Statistic::Pedometer;
			vm_mutex_lock(&mutexMessage);
			messageQueue.addMessage(currentStateStart,currentTimeStamp,messageType,statisticInfo);
			vm_mutex_unlock(&mutexMessage);

			sportingNow=newestState;
			stepCount=0;
			LDateTime.getRtc(&currentStateStart);
		}

		if(sportingNow)
		{
			if(judgeFootstep(acceleration))
			{
				stepCount++;
				Serial.println("A step!");
			}
		}
		else
		{
			//TODO:
			//Codes about sleep monitoring
		}

		uint32_t now=millis();
		uint32_t toDelay=(now-loopStart)%20;
		int ratio=(now-loopStart)/20;
		if(!toDelay)
			toDelay=20;
		loopStart+=20*ratio;
		vm_thread_sleep(toDelay);
	}
	return 0;
}

VMINT32 dataCollector(VM_THREAD_HANDLE thread_handle,void *userData)
{
	int readings[3];

	uint32_t loopStart=millis();

	while(true)
	{
		getReading(readings);

		vm_mutex_lock(&mutexSensor);
		vm_mutex_lock(&mutexSensorDataWrite);

		memcpy((void*)globalReadings,readings,sizeof(readings));
		acceleration=sqrt(readings[0]*readings[0]/4096.0+readings[1]*readings[1]/4096.0+readings[2]*readings[2]/4096.0);

		digitalWrite(13,LOW);

		vm_mutex_unlock(&mutexSensorDataWrite);
		vm_mutex_unlock(&mutexSensor);

		uint32_t now=millis();
		uint32_t toDelay=(now-loopStart)%20;
		int ratio=(now-loopStart)/20;
		if(!toDelay)
			toDelay=20;
		loopStart+=20*ratio;

		vm_thread_sleep(toDelay);
	}
	return 0;
}

VMINT32 fallAlarmSender(VM_THREAD_HANDLE thread_handle,void *userData)
{
	VM_MSG_STRUCT message;
	Serial.println("alarmsender: 1");
	while(true)
	{
		vm_thread_get_msg(&message);

		Serial.println("alarmsender: 2");

		cJSON *fallAlarm=cJSON_CreateObject();

		unsigned int currentTimeStamp;
		LDateTime.getRtc(&currentTimeStamp);
		cJSON_AddStringToObject(fallAlarm,"type","fall");
		cJSON_AddNumberToObject(fallAlarm,"startTime",currentTimeStamp);
		cJSON_AddNumberToObject(fallAlarm,"endTime",currentTimeStamp);
		cJSON_AddItemToObject(fallAlarm,"statistic",cJSON_CreateObject());
		cJSON_AddItemToObject(fallAlarm,"raw",cJSON_CreateArray());
		cJSON_AddStringToObject(fallAlarm,"comment","User fell");

		char *parsedStr=cJSON_PrintUnformatted(fallAlarm);

		//跌倒警报非常紧急，需要在连接状态下立即发送，而不论跌倒发生在过去多远的时刻
		while(!LBTServer.connected())
			vm_thread_sleep(3000);

		LBTServer.write(parsedStr);
		LBTServer.write('\x1F');
		cJSON_Delete(fallAlarm);
		vm_free(parsedStr);

		Serial.println("alarmsender: 3");
	}
	return 0;
}

VMINT32 blueToothConnector(VM_THREAD_HANDLE thread_handle,void *userData)
{
	int noConnectionCount=0;
	int sleepTime=3000;

	while(true)
	{
		if(!LBTServer.connected())
		{
			Serial.println("connector: not connected");
			//没有连接，等待连接
			if(LBTServer.accept(30))
			{
				Serial.println("connector: established");
				vm_signal_post(bluetoothOperationPermission);
				noConnectionCount=0;
				sleepTime=1000;
			}
			else
			{
				Serial.println("connector: timeout");
				noConnectionCount++;
				//多次无连接可认为长时间不会有连接
				if(noConnectionCount>=5)
					sleepTime=10000;
				else
					sleepTime=1000;
			}
		}
		//有连接则恢复较快检查频率
		else
		{
			Serial.println("connector: already connected");
			vm_signal_post(bluetoothOperationPermission);
			noConnectionCount=0;
			sleepTime=2000;
		}
		Serial.println("connector: check later");
		vm_thread_sleep(sleepTime);
	}
	return 0;
}

VMINT32 blueToothReceiver(VM_THREAD_HANDLE thread_handle,void *userData)
{
	char buffer[128];

	while(true)
	{
		Serial.println("receiver: waiting signal");
		vm_signal_wait(bluetoothOperationPermission);
		Serial.println("receiver: signal got");

		//有内容待读取，这里假定一条正常消息的长度不会小于50B
		if(LBTServer.connected()&&LBTServer.available()>50)
		{
			int readLength=LBTServer.readBytesUntil('\x1F',buffer,128);
			buffer[readLength]='\0';

			cJSON *jsonObject=cJSON_Parse(buffer);
			//时间校准指令
			if(strcmp(cJSON_GetObjectItem(jsonObject,"type")->valuestring,"timesync")==0)
			{
				int newTime=cJSON_GetObjectItem(jsonObject,"time")->valueint;
				datetimeInfo dateTimeInfo;

				timestamp_t timestampStruct;
				timestampStruct.sec=newTime;
				timestampStruct.nsec=0;
				timestampStruct.offset=0;

				char YMDHmsBuffer[36];
				timestamp_format(YMDHmsBuffer,36,&timestampStruct);
				sscanf(YMDHmsBuffer,"%d-%d-%dT%d:%d:%dZ",&dateTimeInfo.year,&dateTimeInfo.mon,&dateTimeInfo.day,&dateTimeInfo.hour,&dateTimeInfo.min,&dateTimeInfo.sec);
				LDateTime.setTime(&dateTimeInfo);
			}
			//正常的操作指令
			else
			{
				//即时指令
				if(cJSON_GetObjectItem(jsonObject,"time")->valueint<0)
				{
					bool beep=cJSON_GetObjectItem(jsonObject,"beep")->type;
					bool vibrate=cJSON_GetObjectItem(jsonObject,"vibration")->type;

					//TODO: 操作蜂鸣器与扬声器
				}
				//操作是定时指令
				else
				{
					CommandItem newItem;
					newItem.timeStamp=cJSON_GetObjectItem(jsonObject,"time")->valueint;
					newItem.beep=cJSON_GetObjectItem(jsonObject,"beep")->type;
					newItem.vibration=cJSON_GetObjectItem(jsonObject,"vibration")->type;

					vm_mutex_lock(&mutexCommand);
					commandHeap.push(&newItem);
					vm_mutex_unlock(&mutexCommand);
				}
			}

			cJSON_Delete(jsonObject);
			vm_thread_sleep(1000);
		}
	}
	return 0;
}

VMINT32 blueToothTransmitter(VM_THREAD_HANDLE thread_handle,void *userData)
{
	while(true)
	{
		int messageSentCount=messageQueue.trySend();

		Serial.iprintf("transmiter: %d stored message(s) sent.\n",messageSentCount);
		vm_thread_sleep(10000);
	}
	return 0;
}

void loop()
{
	unsigned int now;

	LDateTime.getRtc(&now);
	CommandItem tmp;

	vm_mutex_lock(&mutexCommand);
	commandHeap.peak(&tmp);
	if(tmp.timeStamp<=now)
	{
		commandHeap.pop();
		//Operate something else
	}
	vm_mutex_unlock(&mutexCommand);

	delay(5000);
}
