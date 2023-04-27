#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"

#if UE_TRACE_ENABLED
struct FPlatformFileTrace
{
    static FORCEINLINE void BeginOpen(const core::string_ref& Path);
    static FORCEINLINE void EndOpen(UInt64 FileHandle);
    static FORCEINLINE void FailOpen(const core::string_ref& Path);
    static FORCEINLINE void BeginReOpen(UInt64 OldFileHandle);
    static FORCEINLINE void EndReOpen(UInt64 NewFileHandle);
    static FORCEINLINE void BeginClose(UInt64 FileHandle);
    static FORCEINLINE void EndClose(UInt64 FileHandle);
    static FORCEINLINE void FailClose(UInt64 FileHandle);
    static FORCEINLINE void BeginRead(UInt64 ReadHandle, UInt64 FileHandle, UInt64 Offset, UInt64 Size);
    static FORCEINLINE void EndRead(UInt64 ReadHandle, UInt64 SizeRead);
    static FORCEINLINE void BeginWrite(UInt64 WriteHandle, UInt64 FileHandle, UInt64 Offset, UInt64 Size);
    static FORCEINLINE void EndWrite(UInt64 WriteHandle, UInt64 SizeWritten);

    static UInt32 GetOpenFileHandleCount();
};

#define TRACE_PLATFORMFILE_BEGIN_OPEN(Path) \
	FPlatformFileTrace::BeginOpen(Path);

#define TRACE_PLATFORMFILE_END_OPEN(FileHandle) \
	FPlatformFileTrace::EndOpen(UInt64(FileHandle));

#define TRACE_PLATFORMFILE_FAIL_OPEN(Path) \
	FPlatformFileTrace::FailOpen(Path);

#define TRACE_PLATFORMFILE_BEGIN_REOPEN(OldFileHandle) \
	FPlatformFileTrace::BeginReOpen(UInt64(OldFileHandle));

#define TRACE_PLATFORMFILE_END_REOPEN(NewFileHandle) \
	FPlatformFileTrace::EndReOpen(UInt64(NewFileHandle));

#define TRACE_PLATFORMFILE_BEGIN_CLOSE(FileHandle) \
	FPlatformFileTrace::BeginClose(UInt64(FileHandle));

#define TRACE_PLATFORMFILE_END_CLOSE(FileHandle) \
	FPlatformFileTrace::EndClose(UInt64(FileHandle));

#define TRACE_PLATFORMFILE_FAIL_CLOSE(FileHandle) \
	FPlatformFileTrace::FailClose(UInt64(FileHandle));

#define TRACE_PLATFORMFILE_BEGIN_READ(ReadHandle, FileHandle, Offset, Size) \
	FPlatformFileTrace::BeginRead(UInt64(ReadHandle), UInt64(FileHandle), Offset, Size);

#define TRACE_PLATFORMFILE_END_READ(ReadHandle, SizeRead) \
	FPlatformFileTrace::EndRead(UInt64(ReadHandle), SizeRead);

#define TRACE_PLATFORMFILE_BEGIN_WRITE(WriteHandle, FileHandle, Offset, Size) \
	FPlatformFileTrace::BeginWrite(UInt64(WriteHandle), UInt64(FileHandle), Offset, Size);

#define TRACE_PLATFORMFILE_END_WRITE(WriteHandle, SizeWritten) \
	FPlatformFileTrace::EndWrite(UInt64(WriteHandle), SizeWritten);
#else
#define TRACE_PLATFORMFILE_BEGIN_OPEN(Path)
#define TRACE_PLATFORMFILE_END_OPEN(FileHandle)
#define TRACE_PLATFORMFILE_BEGIN_REOPEN(OldFileHandle)
#define TRACE_PLATFORMFILE_END_REOPEN(NewFileHandle)
#define TRACE_PLATFORMFILE_FAIL_OPEN(Path)
#define TRACE_PLATFORMFILE_BEGIN_CLOSE(FileHandle)
#define TRACE_PLATFORMFILE_END_CLOSE(FileHandle)
#define TRACE_PLATFORMFILE_FAIL_CLOSE(FileHandle)
#define TRACE_PLATFORMFILE_BEGIN_READ(ReadHandle, FileHandle, Offset, Size)
#define TRACE_PLATFORMFILE_END_READ(ReadHandle, SizeRead)
#define TRACE_PLATFORMFILE_BEGIN_WRITE(WriteHandle, FileHandle, Offset, Size)
#define TRACE_PLATFORMFILE_END_WRITE(WriteHandle, SizeWritten)
#endif
