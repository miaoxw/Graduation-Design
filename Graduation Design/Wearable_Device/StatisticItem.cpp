#include <stddef.h>

#include <LBTServer.h>
#include <vmthread.h>

#include "cJSON.h"
#include "StatisticItem.h"

using Statistic::StatisticType;
using Statistic::QUEUE_SIZE;

extern vm_thread_mutex_struct mutexMessage;

Statistic::SendMessageQueue::SendMessageQueue()
{
	currentCursor=0;
	count=0;

	for(int i=0;i<QUEUE_SIZE;i++)
	{
		messages[i].startTime=messages[i].endTime=0;
		messages[i].type=Pedometer;
		messages[i].statisticInfo=NULL;
	}
}

Statistic::SendMessageQueue::~SendMessageQueue()
{
	for(int i=0;i<QUEUE_SIZE;i++)
		if(messages[i].statisticInfo!=NULL)
			cJSON_Delete(messages[i].statisticInfo);
}

bool Statistic::SendMessageQueue::addMessage(unsigned int startTime,unsigned int endTime,StatisticType type,cJSON *statisticsInfo)
{
	bool ret=true;
	int posToWrite=(currentCursor+count)%QUEUE_SIZE;

	//此位置还有未写到蓝牙串口的数据，需要覆盖
	if(messages[posToWrite].statisticInfo!=NULL)
	{
		cJSON_Delete(messages[posToWrite].statisticInfo);
		messages[posToWrite].statisticInfo=NULL;
	}

	messages[posToWrite].startTime=startTime;
	messages[posToWrite].endTime=endTime;
	messages[posToWrite].type=type;

	if(statisticsInfo==NULL)
	{
		messages[posToWrite].statisticInfo=cJSON_CreateObject();
		ret=false;
	}
	else
	{
		messages[posToWrite].statisticInfo=statisticsInfo;
		ret=true;
	}

	++count;
	if(count>QUEUE_SIZE)
	{
		int delta=count-QUEUE_SIZE;
		count-=delta;
		currentCursor=(currentCursor+delta)%QUEUE_SIZE;
	}

	return ret;
}

char *Statistic::SendMessageQueue::popOne()
{
	char *ret;

	vm_mutex_lock(&mutexMessage);
	if(count>0)
	{
		cJSON *messageToSend=cJSON_CreateObject();

		switch(messages[currentCursor].type)
		{
			case Pedometer:
				cJSON_AddStringToObject(messageToSend,"type","pedometer");
				break;
			case SleepAnalysis:
				cJSON_AddStringToObject(messageToSend,"type","sleep");
				break;
			default:
				break;
		}

		cJSON_AddNumberToObject(messageToSend,"startTime",messages[currentCursor].startTime);
		cJSON_AddNumberToObject(messageToSend,"endTime",messages[currentCursor].endTime);
		cJSON_AddItemToObject(messageToSend,"statistic",messages[currentCursor].statisticInfo);
		cJSON_AddItemToObject(messageToSend,"raw",cJSON_CreateArray());
		cJSON_AddStringToObject(messageToSend,"comment","This is a comment");

		char *parsedStr=cJSON_PrintUnformatted(messageToSend);
		cJSON_Delete(messageToSend);
		messages[currentCursor].statisticInfo=NULL;

		currentCursor=(currentCursor+1)%QUEUE_SIZE;
		count--;

		ret=parsedStr;
	}
	else
		ret=0;

	vm_mutex_unlock(&mutexMessage);
	return ret;
}