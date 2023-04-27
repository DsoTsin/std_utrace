#include "Prefix.h"
#include <stdarg.h>
#include "../TraceLog/Private/Trace/Platform.h"

dynamic_array<core::string> g_commandArgs;

namespace core 
{
	string FormatString(const char* format, ...)
	{
		static thread_local char buffer[2049] = {0};
        va_list args;
        va_start(args, format);
        int result = vsnprintf(buffer, 2049, format, args);
        va_end(args);
		if (result < 2049)
		{
			return string(buffer, result);
		}
		else
		{
			char* allocBuffer = (char*)malloc(result + 1);
			if (allocBuffer != NULL)
			{
				allocBuffer[result] = 0;
				va_list args;
				va_start(args, format);
				result = vsnprintf(allocBuffer, result + 1, format, args);
				va_end(args);
				string retString(allocBuffer, result);
				free(allocBuffer);
				return retString;
			}
			else
			{
				return "";
			}
		}
	}

	void ParseCommandline(const char* commandLine, unsigned int length)
	{
		if (!commandLine || length == 0)
			return;
		g_commandArgs.clear();
		unsigned int offset = 0;
		while (offset < length)
		{
			// -Arg
			// -Arg=Value
			// -Arg=" "
			// " "
			// \" \"
#define CUR_CHAR commandLine[offset]
#define IS_CUR_CHAR_SPACE (CUR_CHAR == ' ' || CUR_CHAR == '\t' || CUR_CHAR == '\n')
#define SKIP_WHITE while (offset < length && IS_CUR_CHAR_SPACE) offset++
			SKIP_WHITE;
			if (offset == length)
				break;
			unsigned int start = offset;
			unsigned int end = start;
			if (CUR_CHAR == '\"')
			{
				offset++;
				while (offset < length && CUR_CHAR != '\"') offset++;
				if (CUR_CHAR == '\"')
					offset++;
				end = offset;
			}
			else
			{
				while (offset < length && (!IS_CUR_CHAR_SPACE && CUR_CHAR != '\"')) offset++;
				if (CUR_CHAR=='\"')
				{
					offset++;
					while (offset < length && CUR_CHAR != '\"') offset++;					
					if (CUR_CHAR == '\"')
						offset++;
					end = offset;
				}
				else
				{
					end = offset;
				}
			}
			if (end > start)
			{
				g_commandArgs.push_back(core::string(commandLine + start, end - start));
			}
			offset++;
#undef SKIP_WHITE
#undef IS_CUR_CHAR_SPACE
#undef CUR_CHAR
		}
	}
}

bool ParseARGV(const char * arg, core::string& value)
{
	auto expArg = core::FormatString("-%s=", arg);
	for (auto& commandArg: g_commandArgs)
	{
		if (!strncmp(expArg.c_str(), *commandArg, expArg.length()))
		{
			value = core::string(*commandArg + expArg.length(), commandArg.length() - expArg.length());
			return true;
		}
	}
	return false;
}

bool HasARGV(const char* arg)
{
	auto expArg = core::FormatString("-%s", arg);
	auto expArg2 = expArg + "=";
	for (auto& commandArg: g_commandArgs)
	{
		if (expArg == commandArg || !strncmp(expArg2.c_str(), *commandArg, expArg2.length()))
		{
			return true;
		}
	}
	return false;
}

int StringToInt(const core::string& str)
{
	return atoi(str.c_str());
}

ChainedAllocator::ChainedAllocator()
	: head(nullptr)
	, list(nullptr)
{
}

ChainedAllocator::~ChainedAllocator()
{
	auto node = head;
	while (node) {
		free(node->ptr);
		node->size = 0;
		auto nxt_node = node->next;
		free(node);
		node = nxt_node;
	}
	head = nullptr;
	list = nullptr;
}

void* ChainedAllocator::Allocate(size_t count, size_t size)
{
	if (head == nullptr)
	{
		head = (Block*)malloc(sizeof(Block));
		head->size = count * size;
		head->ptr = malloc(head->size);
		head->next = NULL;
		list = head;
	}
	else
	{
		auto newblk = (Block*)malloc(sizeof(Block));
		newblk->size = count * size;
		newblk->ptr = malloc(newblk->size);
		newblk->next = NULL;
		list->next = newblk;
		list = newblk;
	}

	return list->ptr;
}

unsigned int Thread::mainThreadId = UE::Trace::Private::ThreadGetId();

Thread::Thread()
	: m_handle(0)
{
}

Thread::~Thread()
{
	if (m_handle)
	{
		UE::Trace::Private::ThreadDestroy(m_handle);
		m_handle = 0;
	}
}

void Thread::SetName(const char* Name)
{
	m_name.assign(Name);
}

void Thread::Run(void* (*entry_point)(void*), void* data, UInt32 stackSize)
{
	m_handle = UE::Trace::Private::ThreadCreate(nullptr, entry_point, data, stackSize);
}

void Thread::WaitForExit()
{
	if (m_handle)
	{
		UE::Trace::Private::ThreadJoin(m_handle);
	}
}