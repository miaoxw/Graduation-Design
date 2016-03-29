#ifndef COMMAND_H
#define COMMAND_H
namespace Command
{
	struct CommandItem
	{
		unsigned int timeStamp;
		bool vibration,beep;
	};

	class CommandHeap
	{
		public:
			CommandHeap();
			~CommandHeap();
			bool empty();
			bool push(const CommandItem *newCommand);
			bool peak(CommandItem* const dest);
			bool pop(CommandItem* const dest);

		private:
			int capacity;
			int size;
			CommandItem *root;
	};

	CommandHeap::CommandHeap()
	{
		root=(CommandItem*)malloc(sizeof(CommandItem)*32);
		size=0;
		capacity=32;
	}

	CommandHeap::~CommandHeap()
	{
		free(root);
	}

	bool CommandHeap::empty()
	{
		return size==0;
	}

	bool CommandHeap::push(const CommandItem *newCommand)
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

	bool CommandHeap::peak(CommandItem* const dest)
	{
		if(!empty())
		{
			*dest=root[0];
			return true;
		}
		else
			return false;
	}

	bool CommandHeap::pop(CommandItem* const dest=NULL)
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
}
#endif // !COMMAND_H

