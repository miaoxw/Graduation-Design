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
		return ++stableCount<1500000;
	}
}
