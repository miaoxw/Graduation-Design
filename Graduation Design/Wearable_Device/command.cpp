#include "command.h"
#include "vmthread.h"

using Command::CommandItem;
//#define NULL 0

Command::CommandHeap::CommandHeap()
{
	root=(CommandItem*)vm_malloc(sizeof(CommandItem)*32);
	size=0;
	capacity=32;
}

Command::CommandHeap::~CommandHeap()
{
	vm_free(root);
}

bool Command::CommandHeap::empty()
{
	return size==0;
}

bool Command::CommandHeap::push(const CommandItem *newCommand)
{
	if(size>=capacity)
		return false;
	else
	{
		size++;

		int j=size-1;
		int i=(j-1)/2;
		while(j>0)
		{
			if(root[i].timeStamp<newCommand->timeStamp)
				break;
			else
			{
				root[j]=root[i];
				j=i;
				i=(j-1)/2;
			}
		}
		root[j]=*newCommand;
		return true;
	}
}

bool Command::CommandHeap::peak(CommandItem* const dest)
{
	if(!empty())
	{
		*dest=root[0];
		return true;
	}
	else
		return false;
}

bool Command::CommandHeap::pop(CommandItem* const dest)
{
	if(!empty())
	{
		if(dest!=NULL)
			*dest=root[0];
		const CommandItem tmp=root[size-1];
		size--;

		int i=0;
		for(int j=2+i+1;j<size;j=2*j+1)
		{
			if(j<size-1&&root[j].timeStamp>root[j+1].timeStamp)
				j++;
			if(tmp.timeStamp<=root[j].timeStamp)
				break;
			else
			{
				root[i]=root[j];
				i=j;
			}
		}
		root[i]=tmp;
		return true;
	}
	else
		return false;
}
