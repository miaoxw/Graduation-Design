#ifndef _NOTIFICATION_h
#define _NOTIFICATION_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

namespace Notification
{
	enum NotificationType
	{
		DoNothing=0,
		VibrateOnly,
		BeepOnly,
		BeepAndVibrate
	};

	void beepAndVibrate();
	void beepOnly();
	void doNothing();
	void vibrateOnly();
}

#endif

