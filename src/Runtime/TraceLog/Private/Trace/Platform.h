// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"

#if UE_TRACE_ENABLED

namespace UE {
namespace Trace {
namespace Private {

////////////////////////////////////////////////////////////////////////////////
UPTRINT	ThreadCreate(const ANSICHAR* Name, void (*Entry)());
UPTRINT	ThreadCreate(const ANSICHAR* Name, void* (*entry_point)(void*), void* data, UInt32 stackSize);
void	ThreadSleep(uint32 Milliseconds);
void	ThreadJoin(UPTRINT Handle);
void	ThreadDestroy(UPTRINT Handle);
uint32	ThreadGetId();

FORCEINLINE size_t  GetCallbackFrames(void** outTrace, size_t maxDepth, size_t skipInitial, UInt32& Hash, bool fastCapture = true);

////////////////////////////////////////////////////////////////////////////////
uint64				TimeGetFrequency();
TRACELOG_API uint64	TimeGetTimestamp();

////////////////////////////////////////////////////////////////////////////////
UPTRINT	TcpSocketConnect(const ANSICHAR* Host, uint16 Port);
UPTRINT	TcpSocketListen(uint16 Port);
int32	TcpSocketAccept(UPTRINT Socket, UPTRINT& Out);
bool	TcpSocketHasData(UPTRINT Socket);

////////////////////////////////////////////////////////////////////////////////
int32	IoRead(UPTRINT Handle, void* Data, uint32 Size);
bool	IoWrite(UPTRINT Handle, const void* Data, uint32 Size);
void	IoClose(UPTRINT Handle);

////////////////////////////////////////////////////////////////////////////////
UPTRINT	FileOpen(const ANSICHAR* Path);

} // namespace Private
} // namespace Trace
} // namespace UE

#endif // UE_TRACE_ENABLED
