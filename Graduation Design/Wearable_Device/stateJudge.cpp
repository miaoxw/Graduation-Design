#include <Arduino.h>

#include "stateJudge.h"

using SportsJudge::SportState;
using SportsJudge::Sleeping;
using SportsJudge::Awake;
using SportsJudge::Sporting;

namespace SportsJudge
{
	const double IDLE_THRESHOLD=1.0;
}

static SportState lastState=SportState(Awake);
static int stableCount=0;
static int lyingCount=0;

SportState SportsJudge::getNewState(double acceleration,int accX,int accY,int accZ)
{
	SportState ret;

	//此处设计为类似FSM的判别形式
	switch(lastState)
	{
		case Sleeping:
			{
				double theta=acos(accZ/sqrt(accX*accX+accY+accY+accZ*accZ))*180/PI;
				if(theta<45)
					lyingCount--;
				else
				{
					lyingCount++;
					if(lyingCount>200)
						lyingCount=200;
				}
				if(lyingCount<=0)
				{
					::stableCount=0;
					lyingCount=0;
					ret=SportState(Awake);
				}
			}
			break;
		case Sporting:
			if(acceleration>IDLE_THRESHOLD)
				ret=SportState(Sporting);
			else
			{
				if(++::stableCount>1800)
				{
					::stableCount=0;
					lyingCount=0;
					ret=SportState(Awake);
				}
			}
			break;
		case Awake:
		default:
			{
				if(acceleration>IDLE_THRESHOLD)
				{
					::stableCount=0;
					ret=SportState(Sporting);
				}
				else
				{
					::stableCount++;
					//静置3分钟后判断用户当前位置仰角
					if(::stableCount>3600)
					{
						//这里的代码其实就是fall.cpp中进行点积求夹角的代码简化
						double theta=acos(accZ/sqrt(accX*accX+accY+accY+accZ*accZ))*180/PI;
						if(theta>=60&&theta<=120)
							lyingCount++;
						else
						{
							lyingCount=0;
							::stableCount=0;
						}
						//超过30秒的平躺状态
						if(lyingCount>600)
						{
							::stableCount=0;
							lyingCount=200;
							ret=SportState(Sleeping);
						}
						else
						{
							::stableCount=3000;
							lyingCount=0;
							ret=SportState(Awake);
						}
					}
				}
			}
			break;
	}

	return lastState=ret;
}
