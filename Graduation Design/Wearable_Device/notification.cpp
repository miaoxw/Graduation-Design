#include "notification.h"
#include "Ports.h"

void Notification::beepAndVibrate()
{
	digitalWrite(BUZZER_PORT,HIGH);
	digitalWrite(VIBRATOR_PORT,HIGH);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);
	delay(400);
	digitalWrite(VIBRATOR_PORT,LOW);
	delay(100);
	digitalWrite(BUZZER_PORT,HIGH);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);
	delay(300);
	digitalWrite(VIBRATOR_PORT,HIGH);
	delay(200);
	digitalWrite(BUZZER_PORT,HIGH);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);
	delay(200);
	digitalWrite(VIBRATOR_PORT,LOW);
	delay(500);
	digitalWrite(VIBRATOR_PORT,HIGH);
	delay(300);
	digitalWrite(BUZZER_PORT,HIGH);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);
	delay(100);
	digitalWrite(VIBRATOR_PORT,LOW);
	delay(400);
	digitalWrite(BUZZER_PORT,HIGH);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);
	digitalWrite(VIBRATOR_PORT,HIGH);
	delay(500);
	digitalWrite(BUZZER_PORT,HIGH);
	digitalWrite(VIBRATOR_PORT,LOW);
	delay(100);
	digitalWrite(BUZZER_PORT,LOW);

	digitalWrite(BUZZER_PORT,LOW);
	digitalWrite(VIBRATOR_PORT,LOW);
}

void Notification::beepOnly()
{
	for(int i=1;i<=3;i++)
	{
		digitalWrite(BUZZER_PORT,HIGH);
		delay(100);
		digitalWrite(BUZZER_PORT,LOW);
		delay(500);
		digitalWrite(BUZZER_PORT,HIGH);
		delay(100);
		digitalWrite(BUZZER_PORT,LOW);
		delay(500);
		digitalWrite(BUZZER_PORT,HIGH);
		delay(100);
		digitalWrite(BUZZER_PORT,LOW);
		if(i!=3)
			delay(1000);
	}

	digitalWrite(BUZZER_PORT,LOW);
	digitalWrite(VIBRATOR_PORT,LOW);
}

void Notification::doNothing()
{
}

void Notification::vibrateOnly()
{
	for(int i=1;i<=4;i++)
	{
		digitalWrite(VIBRATOR_PORT,HIGH);
		delay(500);
		digitalWrite(VIBRATOR_PORT,LOW);
		if(i!=4)
			delay(500);
	}

	digitalWrite(BUZZER_PORT,LOW);
	digitalWrite(VIBRATOR_PORT,LOW);
}
