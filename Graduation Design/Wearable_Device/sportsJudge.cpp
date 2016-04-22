#include "sportsJudge.h"

namespace SportsJudge
{
	int stableCount=0;
	const double IDLE_THRESHOLD=1.0;
}

bool SportsJudge::isSporting(double acceleration)
{
	if(acceleration>IDLE_THRESHOLD)
	{
		stableCount=0;
		return true;
	}
	else
	{
		//以50Hz的采样率计算，空闲90秒需要的时间约为1800次连续计数
		return ++stableCount<1800;
	}
}
