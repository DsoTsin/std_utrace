// Copyright Epic Games, Inc. All Rights Reserved.
#include "Prefix.h"
#include "Runtime/TraceLog/Public/Trace/Trace.h"
#include "Runtime/TraceLog/Public/Trace/Trace.inl" // should be Config.h :(

#if UE_TRACE_ENABLED
#include "Runtime/TraceLog/Public/Trace/Detail/Channel.h"

namespace UE {
namespace Trace {

namespace Private
{

////////////////////////////////////////////////////////////////////////////////
void	Writer_MemorySetHooks(AllocFunc, FreeFunc);
void	Writer_Initialize(const FInitializeDesc&);
void	Writer_WorkerCreate();
void	Writer_Shutdown();
void	Writer_Update();
bool	Writer_SendTo(const ANSICHAR*, uint32);
bool	Writer_WriteTo(const ANSICHAR*);
bool	Writer_WriteSnapshotTo(const ANSICHAR*);
bool	Writer_IsTracing();
bool	Writer_Stop();
uint32	Writer_GetThreadId();

extern FStatistics GTraceStatistics;

} // namespace Private



////////////////////////////////////////////////////////////////////////////////
template <int DestSize, typename SRC_TYPE>
static uint32 ToAnsiCheap(ANSICHAR (&Dest)[DestSize], const SRC_TYPE* Src)
{
	const SRC_TYPE* Cursor = Src;
	for (ANSICHAR& Out : Dest)
	{
		Out = ANSICHAR(*Cursor++ & 0x7f);
		if (Out == '\0')
		{
			break;
		}
	}
	Dest[DestSize - 1] = '\0';
	return uint32(UPTRINT(Cursor - Src));
};

////////////////////////////////////////////////////////////////////////////////
void SetMemoryHooks(AllocFunc Alloc, FreeFunc Free)
{
	Private::Writer_MemorySetHooks(Alloc, Free);
}

////////////////////////////////////////////////////////////////////////////////
void Initialize(const FInitializeDesc& Desc)
{
	Private::Writer_Initialize(Desc);
	FChannel::Initialize();
}

////////////////////////////////////////////////////////////////////////////////
void Shutdown()
{
	Private::Writer_Shutdown();
}

////////////////////////////////////////////////////////////////////////////////
void Update()
{
	Private::Writer_Update();
}

////////////////////////////////////////////////////////////////////////////////
void GetStatistics(FStatistics& Out)
{
	Out = Private::GTraceStatistics;
}

////////////////////////////////////////////////////////////////////////////////
bool SendTo(const char* InHost, uint32 Port)
{
	return Private::Writer_SendTo(InHost, Port);
}

////////////////////////////////////////////////////////////////////////////////
bool WriteTo(const char* InPath)
{
	return Private::Writer_WriteTo(InPath);
}

////////////////////////////////////////////////////////////////////////////////
bool IsTracing()
{
	return Private::Writer_IsTracing();
}

////////////////////////////////////////////////////////////////////////////////
bool Stop()
{
	return Private::Writer_Stop();
}

////////////////////////////////////////////////////////////////////////////////
bool IsChannel(const char* ChannelName)
{
	return FChannel::FindChannel(ChannelName) != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
bool ToggleChannel(const char* ChannelName, bool bEnabled)
{
	return FChannel::Toggle(ChannelName, bEnabled);
}

////////////////////////////////////////////////////////////////////////////////
void EnumerateChannels(ChannelIterFunc IterFunc, void* User)
{
	FChannel::EnumerateChannels(IterFunc, User);
}

////////////////////////////////////////////////////////////////////////////////
void StartWorkerThread()
{
	Private::Writer_WorkerCreate();
}


////////////////////////////////////////////////////////////////////////////////
UE_TRACE_CHANNEL_EXTERN(TraceLogChannel)

UE_TRACE_EVENT_BEGIN($Trace, ThreadInfo, NoSync|Important)
	UE_TRACE_EVENT_FIELD(uint32, ThreadId)
	UE_TRACE_EVENT_FIELD(uint32, SystemId)
	UE_TRACE_EVENT_FIELD(int32, SortHint)
	UE_TRACE_EVENT_FIELD(AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN($Trace, ThreadGroupBegin, NoSync|Important)
	UE_TRACE_EVENT_FIELD(AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN($Trace, ThreadGroupEnd, NoSync|Important)
UE_TRACE_EVENT_END()

////////////////////////////////////////////////////////////////////////////////
void ThreadRegister(const char* Name, uint32 SystemId, int32 SortHint)
{
	uint32 ThreadId = Private::Writer_GetThreadId();
	uint32 NameLen = Name ? (uint32)strlen(Name) : 0;
	UE_TRACE_LOG($Trace, ThreadInfo, TraceLogChannel, NameLen * sizeof(ANSICHAR))
		<< ThreadInfo.ThreadId(ThreadId)
		<< ThreadInfo.SystemId(SystemId)
		<< ThreadInfo.SortHint(SortHint)
		<< ThreadInfo.Name(Name, NameLen);
}

////////////////////////////////////////////////////////////////////////////////
void ThreadGroupBegin(const char* Name)
{
	uint32 NameLen = Name ? (uint32)strlen(Name) : 0;
	UE_TRACE_LOG($Trace, ThreadGroupBegin, TraceLogChannel, NameLen * sizeof(ANSICHAR))
		<< ThreadGroupBegin.Name(Name, NameLen);
}

////////////////////////////////////////////////////////////////////////////////
void ThreadGroupEnd()
{
	UE_TRACE_LOG($Trace, ThreadGroupEnd, TraceLogChannel);
}

} // namespace Trace
} // namespace UE

#else

// Workaround for module not having any exported symbols
TRACELOG_API int TraceLogExportedSymbol = 0;

#endif // UE_TRACE_ENABLED
