#include <stddef.h>
#include <string.h>

#include <LBTServer.h>
#include <LDateTime.h>
#include <LTask.h>
#include <vmpwr.h>
#include <vmthread.h>
#include <Wire.h>

#include "cJSON.h"

#include "ADXL345.h"
#include "command.h"
#include "counter.h"
#include "fall.h"
#include "priority.h"
#include "sportsJudge.h"
#include "ThreadStarter.h"

using namespace ADXL345;
using namespace std;
using Pedometer::judgeFootstep;
using fall::judgeFall;
using Command::CommandHeap;
using Command::CommandItem;
using SportsJudge::isSporting;

VM_SIGNAL_ID fallAlarm,sendMessage,bluetoothOperationPermission;
vm_thread_mutex_struct mutexSensorDataWrite,mutexReaderCount,mutexSensor,mutexCommand;
int readCount;
VMUINT32 blueToothTransmitterHandler;
volatile int globalReadings[3];
volatile double acceleration;
CommandHeap commandHeap;

void getReading(int *store)
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

	if(!LBTServer.begin((uint8_t*)"LinkIt ONE"));
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

			cJSON_AddStringToObject(messageToSend,"type","pedometer");
			cJSON_AddNumberToObject(messageToSend,"startTime",currentStateStart);
			cJSON_AddNumberToObject(messageToSend,"endTime",currentTimeStamp);

			cJSON *statisticInfo=cJSON_CreateObject();
			cJSON_AddBoolToObject(statisticInfo,"sporting",sportingNow);
			cJSON_AddNumberToObject(statisticInfo,"steps",stepCount);

			cJSON_AddItemToObject(messageToSend,"statistic",statisticInfo);
			cJSON_AddItemToObject(messageToSend,"raw",cJSON_CreateArray());
			cJSON_AddStringToObject(messageToSend,"comment",sportingNow?"Sporting state ended":"Idle state ended");

			char *parsedStr=cJSON_PrintUnformatted(messageToSend);
			cJSON_Delete(messageToSend);
			vm_thread_send_msg(blueToothTransmitterHandler,0,parsedStr);

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
		if(LBTServer.connected())
		{
			cJSON *fallAlarm=cJSON_CreateObject();

			unsigned int currentTimeStamp;
			LDateTime.getRtc(&currentTimeStamp);
			cJSON_AddStringToObject(fallAlarm,"type","fall");
			cJSON_AddNumberToObject(fallAlarm,"startTime",currentTimeStamp);
			cJSON_AddNumberToObject(fallAlarm,"endTime",currentTimeStamp);
			cJSON_AddItemToObject(fallAlarm,"statistic",cJSON_CreateObject());
			cJSON_AddItemToObject(fallAlarm,"raw",cJSON_CreateArray());
			cJSON_AddStringToObject(fallAlarm,"comment","Just for test");

			char *parsedStr=cJSON_PrintUnformatted(fallAlarm);
			LBTServer.write(parsedStr);
			LBTServer.write('\x1F');
			cJSON_Delete(fallAlarm);
			vm_free(parsedStr);
		}
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
		////有内容待读取，一条正常消息的长度不会小于50B
		//if(LBTServer.available()>50)
		//{
		//	int readLength=LBTServer.readBytesUntil('\x1F',buffer,128);
		//	buffer[readLength]='\0';

		//	cJSON *jsonObject=cJSON_Parse(buffer);
		//	//即时指令
		//	if(cJSON_GetObjectItem(jsonObject,"time")->valueint<0)
		//	{
		//		bool beep=cJSON_GetObjectItem(jsonObject,"beep")->type;
		//		bool vibrate=cJSON_GetObjectItem(jsonObject,"vibration")->type;

		//		//该振动就振动
		//	}
		//	//操作是定时指令
		//	else
		//	{
		//		CommandItem newItem;
		//		newItem.timeStamp=cJSON_GetObjectItem(jsonObject,"time")->valueint;
		//		newItem.beep=cJSON_GetObjectItem(jsonObject,"beep")->type;
		//		newItem.vibration=cJSON_GetObjectItem(jsonObject,"vibration")->type;

		//		vm_mutex_lock(&mutexCommand);
		//		commandHeap.push(&newItem);
		//		vm_mutex_unlock(&mutexCommand);
		//	}

		//	cJSON_Delete(jsonObject);
		//	vm_thread_sleep(1000);
		//}
		Serial.println("receiver: waiting signal");
		vm_signal_wait(bluetoothOperationPermission);
		Serial.println("receiver: signal got");
		if(LBTServer.connected()&&LBTServer.available())
		{
			char buf[100];
			int i=LBTServer.readBytesUntil('\x1F',buf,100);
			buf[i]='\0';
			Serial.iprintf("receiver: i=%d\n",i);
			Serial.iprintf("receiver: content:%s\n",buf);
			char *ret=(char*)vm_malloc(15);
			memcpy(ret,"Hello too!",15);
			vm_thread_send_msg(blueToothTransmitterHandler,0,ret);

			ret=(char*)vm_malloc(15);
			memcpy(ret,"Hello too! 1",15);
			vm_thread_send_msg(blueToothTransmitterHandler,0,ret);

			ret=(char*)vm_malloc(15);
			memcpy(ret,"Hello too! 2",15);
			vm_thread_send_msg(blueToothTransmitterHandler,0,ret);

			ret=(char*)vm_malloc(20);
			memcpy(ret,"Hello t   oo! 3",20);
			vm_thread_send_msg(blueToothTransmitterHandler,0,ret);

			ret=(char*)vm_malloc(15);
			memcpy(ret,"Hello \tto\no! 4",15);
			vm_thread_send_msg(blueToothTransmitterHandler,0,ret);
		}
	}
	return 0;
}

VMINT32 blueToothTransmitter(VM_THREAD_HANDLE thread_handle,void *userData)
{
	blueToothTransmitterHandler=vm_thread_get_current_handle();
	VM_MSG_STRUCT message;

	while(true)
	{
		vm_thread_get_msg(&message);
		Serial.println("transmitter: new message");

		//TODO: 可能需要带超时的信号等待机制以通过蓝牙进行通信

		if(LBTServer.connected())
		{
			int byteSent=LBTServer.write((char*)message.user_data);
			Serial.iprintf("transmiter: \"%s\" is sent, actually %dB.\n",(char*)message.user_data,byteSent+1);
			LBTServer.write('\x1F');
		}
		vm_free(message.user_data);
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
