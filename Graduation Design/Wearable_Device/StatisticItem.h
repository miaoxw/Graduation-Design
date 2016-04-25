// StatisticItem.h

#ifndef _STATISTICITEM_h
#define _STATISTICITEM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

namespace Statistic
{
	const int QUEUE_SIZE=32;

	enum StatisticType
	{
		Pedometer,
		SleepAnalysis
	};

	struct MessageItem
	{
		unsigned int startTime;
		unsigned int endTime;
		StatisticType type;
		cJSON *statisticInfo;
	};

	class SendMessageQueue
	{
		private:
			MessageItem messages[QUEUE_SIZE];
			int currentCursor;
			int count;

		public:
			SendMessageQueue();
			~SendMessageQueue();
			bool addMessage(unsigned int startTime,unsigned int endTime,StatisticType type,cJSON *statisticInfo);
			char *popOne();
	};
}
#endif

