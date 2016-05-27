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
//#include "BTBinding.h"
#include "command.h"
#include "counter.h"
#include "fall.h"
#include "notification.h"
#include "Ports.h"
#include "priority.h"
#include "sleep.h"
#include "stateJudge.h"
#include "StatisticItem.h"
#include "ThreadStarter.h"

using namespace ADXL345;
using namespace Notification;
using namespace std;
//using namespace BluetoothBinding;
using Pedometer::judgeFootstep;
using fall::judgeFall;
using Command::CommandHeap;
using Command::CommandItem;
using Sleep::sleep_conf;
using Sleep::sleep_get_count;
using Sleep::sleep_get_state;
using Sleep::sleep_init;
using Sleep::sleep_on_data;
using SportsJudge::SportState;
using SportsJudge::Awake;
using SportsJudge::Sleeping;
using SportsJudge::Sporting;
using SportsJudge::getNewState;
using Statistic::SendMessageQueue;
using Statistic::StatisticType;

VM_SIGNAL_ID fallAlarm,sendMessage,bluetoothOperationPermission;
vm_thread_mutex_struct mutexSensorDataWrite,mutexReaderCount,mutexSensor,mutexCommand,mutexMessage;
VMUINT32 notifierHandle;
int readCount;
volatile int globalReadings[3];
volatile double acceleration;
CommandHeap commandHeap;
SendMessageQueue messageQueue;

unsigned short m_k_vals[]={15,15,15,8,21,12,13};
struct sleep_conf sleepConfig;

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
	pinMode(BUZZER_PORT,OUTPUT);
	pinMode(VIBRATOR_PORT,OUTPUT);
	pinMode(13,OUTPUT);
	digitalWrite(BUZZER_PORT,LOW);
	digitalWrite(VIBRATOR_PORT,LOW);
	digitalWrite(13,LOW);
	//Serial.begin(57600);

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
	Wire.write(0x9);//���͹��ģ�50Hz
	Wire.endTransmission();

	//TODO: ADXL345�йؾ�ֹ���ѵ��ж�

	Wire.beginTransmission(address);
	Wire.write(INT_ENABLE);
	Wire.write(0x0);//���ж�
	Wire.endTransmission();
	Wire.beginTransmission(address);
	Wire.write(POWER_CTL);
	Wire.write(0x8);//����ģʽ
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

	//��ʼ���ź�����
	readCount=0;
	vm_mutex_create(&mutexReaderCount);
	vm_mutex_create(&mutexSensorDataWrite);
	vm_mutex_create(&mutexSensor);
	vm_mutex_create(&mutexCommand);
	vm_mutex_create(&mutexMessage);
	fallAlarm=vm_signal_init();
	sendMessage=vm_signal_init();
	bluetoothOperationPermission=vm_signal_init();

	//��ʼ��˯�߲���
	sleepConfig.m_hz=50;
	sleepConfig.m_short_period=4;
	sleepConfig.m_long_period=100;
	sleepConfig.m_max_sleep_minc=720;
	sleepConfig.m_ks.m_values=m_k_vals;
	sleepConfig.m_ks.m_len=7;
	sleepConfig.m_ks.m_offset=2;
	sleepConfig.m_threhold.m_webster=4000;
	sleepConfig.m_threhold.m_rovel=30;
	sleepConfig.m_threhold.m_head=15;
	sleepConfig.m_states_period=10;

	//�������������߳�
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
	SportState currentState=SportState(Awake);

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

		//����Ǩ��������
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
			//Serial.println("Falling!");
			vm_signal_post(fallAlarm);
		}

		SportState newestState=getNewState(acceleration,readings[0],readings[1],readings[2]);
		if(newestState!=currentState)
		{
			if(currentState==Sporting&&stepCount==0)
				currentState=SportState(Awake);
			else
			{
				cJSON *messageToSend=cJSON_CreateObject();

				unsigned int currentTimeStamp;
				LDateTime.getRtc(&currentTimeStamp);

				cJSON *statisticInfo=cJSON_CreateObject();
				StatisticType messageType;

				switch(currentState)
				{
					case SportState(Sporting):
						messageType=Statistic::Pedometer;
						cJSON_AddBoolToObject(statisticInfo,"sporting",true);
						cJSON_AddNumberToObject(statisticInfo,"steps",stepCount);
						break;
					case SportState(Awake):
						messageType=Statistic::Pedometer;
						cJSON_AddBoolToObject(statisticInfo,"sporting",false);
						cJSON_AddNumberToObject(statisticInfo,"steps",0);
						break;
					case SportState(Sleeping):
						messageType=Statistic::SleepAnalysis;
						int totalTime=sleep_get_count();
						int identifiedTime[]={0,0,0,0};
						for(int i=0;i<totalTime;i++)
							identifiedTime[sleep_get_state(i)]++;

						cJSON_AddNumberToObject(statisticInfo,"total",totalTime);
						cJSON_AddNumberToObject(statisticInfo,"awake",identifiedTime[0]);
						cJSON_AddNumberToObject(statisticInfo,"lightSleep",identifiedTime[1]);
						cJSON_AddNumberToObject(statisticInfo,"deepSleep",identifiedTime[2]);
						cJSON_AddNumberToObject(statisticInfo,"REM",identifiedTime[3]);
						break;
				}

				vm_mutex_lock(&mutexMessage);
				messageQueue.addMessage(currentStateStart,currentTimeStamp,messageType,statisticInfo);
				vm_mutex_unlock(&mutexMessage);
				vm_signal_post(sendMessage);

				currentState=newestState;
				if(newestState==Sporting)
				{
					stepCount=0;
					Pedometer::init();
				}
				if(newestState==Sleeping)
				{
					sleep_init(&sleepConfig);
				}
				if(newestState==Awake)
				{
					//Nothing to do
					stepCount=0;
				}

				LDateTime.getRtc(&currentStateStart);
			}
		}

		switch(currentState)
		{
			case Sporting:
			case Awake:
				if(judgeFootstep(acceleration))
					stepCount++;
				break;
			case Sleeping:
				sleep_on_data(readings[0],readings[1],readings[2]);
				break;
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

	//Serial.println("alarmsender: 1");
	while(true)
	{
		vm_thread_get_msg(&message);

		//Serial.println("alarmsender: 2");

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

		//���������ǳ���������Ҫ������״̬���������ͣ������۵��������ڹ�ȥ��Զ��ʱ��
		//while(!LBTServer.connected())
		//	vm_thread_sleep(3000);

		LBTServer.write(parsedStr);
		LBTServer.write('\x1F');
		cJSON_Delete(fallAlarm);
		vm_free(parsedStr);

		//Serial.println("alarmsender: 3");
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
			//Serial.println("connector: not connected");
			if(LBTServer.accept(30))
			{
				//Serial.println("connector: established");

				vm_signal_post(bluetoothOperationPermission);
				noConnectionCount=0;
				sleepTime=1000;
			}
			else
			{
				//Serial.println("connector: timeout");
				noConnectionCount++;
				//��������ӿ���Ϊ��ʱ�䲻��������
				if(noConnectionCount>=5)
					sleepTime=10000;
				else
					sleepTime=1000;
			}
			//}
		}
		//��������ָ��Ͽ���Ƶ��
		else
		{
			//Serial.println("connector: already connected");
			vm_signal_post(bluetoothOperationPermission);
			noConnectionCount=0;
			sleepTime=2000;
		}
		//Serial.println("connector: check later");
		vm_thread_sleep(sleepTime);
	}
	return 0;
}

VMINT32 blueToothReceiver(VM_THREAD_HANDLE thread_handle,void *userData)
{
	char buffer[128];

	while(true)
	{
		//Serial.println("receiver: waiting signal");
		vm_signal_wait(bluetoothOperationPermission);
		//Serial.println("receiver: signal got");

		//�����ݴ���ȡ������ٶ�һ��������Ϣ�ĳ��Ȳ���С��50B
		if(LBTServer.connected()&&LBTServer.available()>50)
		{
			int readLength=LBTServer.readBytesUntil('\x1F',buffer,128);
			buffer[readLength]='\0';

			cJSON *jsonObject=cJSON_Parse(buffer);
			//ʱ��У׼ָ��
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
			//�����Ĳ���ָ��
			else
			{
				//��ʱָ��
				if(cJSON_GetObjectItem(jsonObject,"time")->valueint<0)
				{
					bool beep=cJSON_GetObjectItem(jsonObject,"beep")->type;
					bool vibrate=cJSON_GetObjectItem(jsonObject,"vibration")->type;

					//������������������
					NotificationType type=(NotificationType)((beep?1:0)<<1|(vibrate?1:0));
					vm_thread_send_msg(notifierHandle,type,NULL);
				}
				//�����Ƕ�ʱָ��
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
		vm_signal_wait(sendMessage);

		char *JSONString=messageQueue.popOne();

		while(JSONString!=NULL)
		{
			vm_signal_wait(bluetoothOperationPermission);
			LBTServer.write(JSONString);
			LBTServer.write('\x1F');

			vm_free(JSONString);
			JSONString=messageQueue.popOne();
		}
	}
	return 0;
}

VMINT32 notifier(VM_THREAD_HANDLE thread_handle,void *userData)
{
	void (*functionTable[])()={doNothing,vibrateOnly,beepOnly,beepAndVibrate};
	notifierHandle=vm_thread_get_current_handle();
	VM_MSG_STRUCT message;

	while(true)
	{
		vm_thread_get_msg(&message);

		NotificationType notificationType=(NotificationType)message.message_id;
		switch(notificationType)
		{
			case VibrateOnly:
				vibrateOnly();
				break;
			case BeepOnly:
				beepOnly();
				break;
			case BeepAndVibrate:
				beepAndVibrate();
				break;
			case DoNothing:
			default:
				doNothing();
				break;
		}
	}
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
		NotificationType type=(NotificationType)((tmp.beep?1:0)<<1|(tmp.vibration?1:0));
		vm_thread_send_msg(notifierHandle,type,NULL);
	}
	vm_mutex_unlock(&mutexCommand);

	delay(5000);
}
