#ifndef COMMAND_H
#define COMMAND_H

#define NULL 0

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
			bool pop(CommandItem* const dest=NULL);

		private:
			int capacity;
			int size;
			CommandItem *root;
	};
}
#endif // !COMMAND_H

