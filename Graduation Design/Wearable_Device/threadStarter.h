#include <LTask.h>

#ifndef THREAD_STARTER_H
#define THREAD_STARTER_H
struct ThreadStarter
{
	VM_THREAD_FUNC func;
	VMUINT8 priority;
};
#endif // !THREAD_STARTER_H

