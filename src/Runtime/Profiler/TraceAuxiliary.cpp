#include "Prefix.h"

#include "utrace.h"
#include "TraceAuxiliary.h"

#include "GrowOnlyLockFreeHash.h"

#include "UTrace/GPUTrace.h"
#include "UTrace/FileTrace.h"
//#include "UTrace/CounterTrace.h"
#include "UTrace/MemoryTrace.h"

#ifdef _MSC_VER
#pragma warning(disable:4200 4244 4996)
#endif

#if LILITH_UNITY_UNREAL_INSIGHTS
#include <memory>
#include <atomic>
#include <thread>

#include "Runtime/TraceLog/Public/Trace/Trace.h"
#include "Runtime/TraceLog/Public/Trace/Trace.inl"
#include "Runtime/TraceLog/Public/Trace/Detail/EventNode.h"
#include "Runtime/TraceLog/Public/Trace/Detail/Important/ImportantLogScope.h"
#include "Runtime/TraceLog/Public/Trace/Detail/LogScope.h"

#include "Runtime/TraceLog/Private/Trace/Platform.h"

#include "Backtracer.h"

#if PLATFORM_ANDROID
#include <jni.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/system_properties.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <link.h>

#include "Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xhook.h"

typedef void (*Fn_dl_notify)(int, const char* so_name, const char* realpath, ElfW(Addr) base, const ElfW(Phdr)* phdr, ElfW(Half) phnum, void*);
typedef void (*Fn_dl_register_notification)(Fn_dl_notify notify, void*);

core::string GetAndroidProp(const char* propName)
{
    core::string value;
    char buffer[2048] = { 0 };
    if (__system_property_get(propName, buffer) > 0)
    {
        value = buffer;
    }
    return value;
}
#endif

UE_TRACE_EVENT_BEGIN(Diagnostics, Session2, NoSync | Important)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Platform)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, AppName)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, CommandLine)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Branch)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, BuildVersion)
#if PLATFORM_ANDROID
    // TODO: UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, GPU) // GPU Name:
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Model) // ro.product.model
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, SOC) // SOC Name: ro.board.platform
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Brand) // Brand Name: ro.product.brand
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, OSVer) // OSVersion: ro.build.version.release
#endif
    UE_TRACE_EVENT_FIELD(uint32_t, Changelist)
    UE_TRACE_EVENT_FIELD(uint8_t, ConfigurationType)
    UE_TRACE_EVENT_FIELD(uint8_t, TargetType)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(CpuChannel)
UE_TRACE_CHANNEL_DEFINE(GpuChannel)

UE_TRACE_EVENT_BEGIN(GpuProfiler, EventSpec, NoSync|Important)
	UE_TRACE_EVENT_FIELD(UInt32, EventType)
	UE_TRACE_EVENT_FIELD(UInt16[], Name)
UE_TRACE_EVENT_END()

// GPU Index 0
UE_TRACE_EVENT_BEGIN(GpuProfiler, Frame)
	UE_TRACE_EVENT_FIELD(UInt64, CalibrationBias)
	UE_TRACE_EVENT_FIELD(UInt64, TimestampBase)
	UE_TRACE_EVENT_FIELD(UInt32, RenderingFrameNumber)
	UE_TRACE_EVENT_FIELD(UInt8[], Data)
UE_TRACE_EVENT_END()

// GPU Index 1
UE_TRACE_EVENT_BEGIN(GpuProfiler, Frame2)
	UE_TRACE_EVENT_FIELD(UInt64, CalibrationBias)
	UE_TRACE_EVENT_FIELD(UInt64, TimestampBase)
	UE_TRACE_EVENT_FIELD(UInt32, RenderingFrameNumber)
	UE_TRACE_EVENT_FIELD(UInt8[], Data)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CpuProfiler, EventSpec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(UInt32, Id)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, File)
    UE_TRACE_EVENT_FIELD(UInt32, Line)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CpuProfiler, EventBatch, NoSync)
    UE_TRACE_EVENT_FIELD(UInt8[], Data)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CpuProfiler, EndCapture)
    UE_TRACE_EVENT_FIELD(UInt8[], Data)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CpuProfiler, EndThread, NoSync)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(ModuleChannel, "Module information needed for symbols resolution", true)
 
UE_TRACE_EVENT_BEGIN(Diagnostics, ModuleInit, NoSync|Important)
	UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, SymbolFormat)
	UE_TRACE_EVENT_FIELD(UInt8, ModuleBaseShift)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Diagnostics, ModuleLoad, NoSync|Important)
	UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
	UE_TRACE_EVENT_FIELD(UInt64, Base)
	UE_TRACE_EVENT_FIELD(UInt32, Size)
    // Platform specific id for this image, used to match debug files were available
	UE_TRACE_EVENT_FIELD(UInt8[], ImageId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Diagnostics, ModuleUnload, NoSync|Important)
	UE_TRACE_EVENT_FIELD(UInt64, Base)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(PerfettoChannel)

UE_TRACE_EVENT_BEGIN(Perfetto, TracePacketChunk)
	UE_TRACE_EVENT_FIELD(uint16_t, Size)
	UE_TRACE_EVENT_FIELD(uint8_t[], Data)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(CountersChannel)

UE_TRACE_EVENT_BEGIN(Counters, Spec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(uint16_t, Id)
    UE_TRACE_EVENT_FIELD(uint8_t, Type)
    UE_TRACE_EVENT_FIELD(uint8_t, DisplayHint)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Counters, SetValueInt)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(int64_t, Value)
    UE_TRACE_EVENT_FIELD(uint16_t, CounterId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Counters, SetValueFloat)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(double, Value)
    UE_TRACE_EVENT_FIELD(uint16_t, CounterId)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL(FrameChannel)
UE_TRACE_CHANNEL(BookmarkChannel)

UE_TRACE_EVENT_BEGIN(Misc, BookmarkSpec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(const void*, BookmarkPoint)
    UE_TRACE_EVENT_FIELD(int32_t, Line)
    UE_TRACE_EVENT_FIELD(UE::Trace::WideString, FormatString)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, FileName)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Misc, Bookmark)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(const void*, BookmarkPoint)
    UE_TRACE_EVENT_FIELD(uint8_t[], FormatArgs)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Misc, BeginFrame)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(uint8_t, FrameType)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Misc, EndFrame)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(uint8_t, FrameType)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL(ScreenshotChannel)

static int GScreenshotInterval = 100;

UE_TRACE_EVENT_BEGIN(Misc, ScreenshotHeader)
	UE_TRACE_EVENT_FIELD(uint32_t, Id)
	UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
	UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
	UE_TRACE_EVENT_FIELD(uint32_t, Width)
	UE_TRACE_EVENT_FIELD(uint32_t, Height)
	UE_TRACE_EVENT_FIELD(uint32_t, TotalChunkNum)
	UE_TRACE_EVENT_FIELD(uint32_t, Size)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Misc, ScreenshotChunk)
	UE_TRACE_EVENT_FIELD(uint32_t, Id)
	UE_TRACE_EVENT_FIELD(uint32_t, ChunkNum)
	UE_TRACE_EVENT_FIELD(uint16_t, Size)
	UE_TRACE_EVENT_FIELD(uint8_t[], Data)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(TaskChannel);

UE_TRACE_EVENT_BEGIN(TaskTrace, Init)
    UE_TRACE_EVENT_FIELD(uint32_t, Version)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Created)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Launched)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
    UE_TRACE_EVENT_FIELD(UE::Trace::WideString, DebugName)
    UE_TRACE_EVENT_FIELD(bool, Tracked)
    UE_TRACE_EVENT_FIELD(int32_t, ThreadToExecuteOn)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Scheduled)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, SubsequentAdded)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
    UE_TRACE_EVENT_FIELD(uint64_t, SubsequentId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Started)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, NestedAdded)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
    UE_TRACE_EVENT_FIELD(uint64_t, NestedId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Finished)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, Completed)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t, TaskId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, WaitingStarted)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
    UE_TRACE_EVENT_FIELD(uint64_t[], Tasks)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(TaskTrace, WaitingFinished)
    UE_TRACE_EVENT_FIELD(uint64_t, Timestamp)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(LogChannel)

UE_TRACE_EVENT_BEGIN(Logging, LogCategory, NoSync | Important)
    UE_TRACE_EVENT_FIELD(const void*, CategoryPointer)
    UE_TRACE_EVENT_FIELD(uint8_t, DefaultVerbosity)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Logging, LogMessageSpec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(const void*, LogPoint)
    UE_TRACE_EVENT_FIELD(const void*, CategoryPointer)
    UE_TRACE_EVENT_FIELD(int32_t, Line)
    UE_TRACE_EVENT_FIELD(uint8_t, Verbosity)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, FileName)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, FormatString)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Logging, LogMessage, NoSync)
    UE_TRACE_EVENT_FIELD(const void*, LogPoint)
    UE_TRACE_EVENT_FIELD(uint64_t, Cycle)
    UE_TRACE_EVENT_FIELD(uint8_t[], FormatArgs)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL(MetadataChannel);

UE_TRACE_EVENT_BEGIN(MetadataStack, ClearScope, NoSync)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(MetadataStack, SaveStack)
	UE_TRACE_EVENT_FIELD(UInt32, Id)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(MetadataStack, RestoreStack)
	UE_TRACE_EVENT_FIELD(UInt32, Id)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL(AssetMetadataChannel);

UE_TRACE_EVENT_BEGIN(Strings, StaticString, NoSync|Definition64bit)
	UE_TRACE_EVENT_FIELD(UE::Trace::WideString, DisplayWide)
	UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, DisplayAnsi)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Strings, FName, NoSync|Definition32bit)
	UE_TRACE_EVENT_FIELD(UE::Trace::WideString, DisplayWide)
	UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, DisplayAnsi)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Metadata, Asset)
    // TODO: UE_TRACE_EVENT_REFERENCE_FIELD(Strings, FName, Name)
    // TODO: UE_TRACE_EVENT_REFERENCE_FIELD(Strings, FName, Class)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL(FileChannel)

UE_TRACE_EVENT_BEGIN(PlatformFile, BeginOpen)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Path)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, EndOpen)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, FileHandle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, BeginReOpen)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, OldFileHandle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, EndReOpen)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, NewFileHandle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, BeginClose)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, FileHandle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, EndClose)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, BeginRead)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, ReadHandle)
    UE_TRACE_EVENT_FIELD(UInt64, FileHandle)
    UE_TRACE_EVENT_FIELD(UInt64, Offset)
    UE_TRACE_EVENT_FIELD(UInt64, Size)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, EndRead)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, ReadHandle)
    UE_TRACE_EVENT_FIELD(UInt64, SizeRead)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, BeginWrite)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, WriteHandle)
    UE_TRACE_EVENT_FIELD(UInt64, FileHandle)
    UE_TRACE_EVENT_FIELD(UInt64, Offset)
    UE_TRACE_EVENT_FIELD(UInt64, Size)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(PlatformFile, EndWrite)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
    UE_TRACE_EVENT_FIELD(UInt64, WriteHandle)
    UE_TRACE_EVENT_FIELD(UInt64, SizeWritten)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(MemAllocChannel, "Memory allocations", true)

UE_TRACE_EVENT_BEGIN(Memory, Init, NoSync | Important)
    UE_TRACE_EVENT_FIELD(UInt32, MarkerPeriod)
    UE_TRACE_EVENT_FIELD(UInt8, Version)
    UE_TRACE_EVENT_FIELD(UInt8, MinAlignment)
    UE_TRACE_EVENT_FIELD(UInt8, SizeShift)
    UE_TRACE_EVENT_FIELD(UInt8, Mode)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, Marker)
    UE_TRACE_EVENT_FIELD(UInt64, Cycle)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, Alloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Size)
    UE_TRACE_EVENT_FIELD(UInt8, AlignmentPow2_SizeLower)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, AllocSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Size)
    UE_TRACE_EVENT_FIELD(UInt8, AlignmentPow2_SizeLower)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, AllocVideo)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Size)
    UE_TRACE_EVENT_FIELD(UInt8, AlignmentPow2_SizeLower)
UE_TRACE_EVENT_END()
/*
// void *addr, size_t length, int prot, int flags, int fd, off_t offset
UE_TRACE_EVENT_BEGIN(Memory, Mmap)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt64, InAddress)
    UE_TRACE_EVENT_FIELD(UInt64, Length)
    UE_TRACE_EVENT_FIELD(UInt32, Prot)
    UE_TRACE_EVENT_FIELD(UInt32, Flags)
    UE_TRACE_EVENT_FIELD(SInt32, Fd)
    UE_TRACE_EVENT_FIELD(UInt64, Offset)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()
*/
UE_TRACE_EVENT_BEGIN(Memory, Free)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()
/*
//void *addr, size_t length
UE_TRACE_EVENT_BEGIN(Memory, Munmap)
    UE_TRACE_EVENT_FIELD(SInt32, Result)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt64, Length)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()
*/
UE_TRACE_EVENT_BEGIN(Memory, FreeSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, FreeVideo)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, ReallocAlloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Size)
    UE_TRACE_EVENT_FIELD(UInt8, AlignmentPow2_SizeLower)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, ReallocAllocSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Size)
    UE_TRACE_EVENT_FIELD(UInt8, AlignmentPow2_SizeLower)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, ReallocFree)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, ReallocFreeSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
UE_TRACE_EVENT_END()

// Set object name for Asset
UE_TRACE_EVENT_BEGIN(Memory, MarkObject)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, HeapSpec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(UInt32, Id)
    UE_TRACE_EVENT_FIELD(UInt32, ParentId)
    UE_TRACE_EVENT_FIELD(UInt16, Flags)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, HeapMarkAlloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt16, Flags)
    UE_TRACE_EVENT_FIELD(UInt32, Heap)
    UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, HeapUnmarkAlloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
    UE_TRACE_EVENT_FIELD(UInt32, Heap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, TagSpec, Important | NoSync)
    UE_TRACE_EVENT_FIELD(SInt32, Tag)
    UE_TRACE_EVENT_FIELD(SInt32, Parent)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Display)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, MemoryScope, NoSync)
    UE_TRACE_EVENT_FIELD(SInt32, Tag)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, MemoryScopePtr, NoSync)
    UE_TRACE_EVENT_FIELD(UInt64, Ptr)
UE_TRACE_EVENT_END()

UE_TRACE_CHANNEL_DEFINE(VulkanChannel, "Vulkan Event Markers", true)

UE_TRACE_EVENT_BEGIN(Vulkan, SetRenderPassName)
    UE_TRACE_EVENT_FIELD(UInt64, RenderPass)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Vulkan, SetFrameBufferName)
    UE_TRACE_EVENT_FIELD(UInt64, FrameBuffer)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Vulkan, SetCommandBufferName)
    UE_TRACE_EVENT_FIELD(UInt64, CommandBuffer)
    UE_TRACE_EVENT_FIELD(UE::Trace::AnsiString, Name)
UE_TRACE_EVENT_END()

using namespace utrace;

// If the layout of the above events is changed, bump this version number.
// version 1: Initial version (UE 5.0, UE 5.1)
// version 2: Added CallstackId for Free events and also for HeapMarkAlloc, HeapUnmarkAlloc events (UE 5.2).
constexpr UInt8 MemoryTraceVersion = 2;

static std::atomic_bool GIsInitialized{false};
static std::atomic_bool GIsModuleInitialized{false};

bool GCaptureCallStack = true;
bool GUseFastCallStackTrace = true;

#ifdef _WIN32
#else
static pthread_key_t GNeedHookMalloc = NULL;
#endif

extern dynamic_array<core::string> g_commandArgs;

class FTlsAutoCleanup
{
public:
    /** Virtual destructor. */
    virtual ~FTlsAutoCleanup()
    {}

    /** Register this instance to be auto-cleanup. */
    void Register();
};

#if UE_TRACE_ENABLED
struct FCpuProfilerTraceInternal
{
    enum
    {
        MaxBufferSize = 256,
        MaxEncodedEventSize = 15, // 10 + 5
        FullBufferThreshold = MaxBufferSize - MaxEncodedEventSize,
    };

    struct hash
    {
        uint32_t operator()(const char* in) const
        {
            UInt32 Hash = 5381;
            for (const char* c = in; *c; ++c)
            {
                UInt32 LowerC = *c | 0x20;
                Hash = ((Hash << 5) + Hash) + LowerC;
            }
            return Hash;
        }
    };

    struct equal
    {
        bool operator()(const char* a, const char* b) const
        {
            if (a == b)
                return true;
            else
                return strcmp(a, b) == 0;
        }
    };

    struct FThreadBuffer
        : public FTlsAutoCleanup
    {
        FThreadBuffer()
            : ChainAllocator()
            , DynamicAnsiScopeNamesMap{}
        {
        }

        virtual ~FThreadBuffer()
        {
            UE_TRACE_LOG(CpuProfiler, EndThread, CpuChannel);
            // Clear the thread buffer pointer. In the rare event of there being scopes in the destructors of other
            // FTLSAutoCleanup instances. In that case a new buffer is created for that event only. There is no way of
            // controlling the order of destruction for FTLSAutoCleanup types.
            ThreadBuffer = nullptr;
        }

        uint64_t LastCycle = 0;
        uint16_t BufferSize = 0;
        uint8_t Buffer[MaxBufferSize];
        ChainedAllocator   ChainAllocator;
        core::hash_map<const char*, UInt32, hash, equal> DynamicAnsiScopeNamesMap;
    };

    uint32_t static GetNextSpecId();
    FORCENOINLINE static FThreadBuffer* CreateThreadBuffer();
    FORCENOINLINE static void FlushThreadBuffer(FThreadBuffer* ThreadBuffer);
    FORCENOINLINE static void EndCapture(FThreadBuffer* ThreadBuffer);

    static thread_local uint32_t ThreadDepth;
    static thread_local FThreadBuffer* ThreadBuffer;
};

thread_local uint32_t FCpuProfilerTraceInternal::ThreadDepth = 0;
thread_local FCpuProfilerTraceInternal::FThreadBuffer* FCpuProfilerTraceInternal::ThreadBuffer = nullptr;

FCpuProfilerTraceInternal::FThreadBuffer* FCpuProfilerTraceInternal::CreateThreadBuffer()
{
    ThreadBuffer = new FThreadBuffer();
    ThreadBuffer->Register();
    return ThreadBuffer;
}

void FCpuProfilerTraceInternal::FlushThreadBuffer(FThreadBuffer* InThreadBuffer)
{
    UE_TRACE_LOG(CpuProfiler, EventBatch, true)
        << EventBatch.Data(InThreadBuffer->Buffer, InThreadBuffer->BufferSize);
    InThreadBuffer->BufferSize = 0;
    InThreadBuffer->LastCycle = 0;
}

void FCpuProfilerTraceInternal::EndCapture(FThreadBuffer* InThreadBuffer)
{
    UE_TRACE_LOG(CpuProfiler, EndCapture, true)
        << EndCapture.Data(InThreadBuffer->Buffer, InThreadBuffer->BufferSize);
    InThreadBuffer->BufferSize = 0;
    InThreadBuffer->LastCycle = 0;
}

#define CPUPROFILERTRACE_OUTPUTBEGINEVENT_PROLOGUE() \
	++FCpuProfilerTraceInternal::ThreadDepth; \
	FCpuProfilerTraceInternal::FThreadBuffer* ThreadBuffer = FCpuProfilerTraceInternal::ThreadBuffer; \
	if (!ThreadBuffer) \
	{ \
		ThreadBuffer = FCpuProfilerTraceInternal::CreateThreadBuffer(); \
	} \

#define CPUPROFILERTRACE_OUTPUTBEGINEVENT_EPILOGUE() \
	uint64_t Cycle = UE::Trace::Private::TimeGetTimestamp(); \
	uint64_t CycleDiff = Cycle - ThreadBuffer->LastCycle; \
	ThreadBuffer->LastCycle = Cycle; \
	uint8_t* BufferPtr = ThreadBuffer->Buffer + ThreadBuffer->BufferSize; \
	FTraceUtils::Encode7bit((CycleDiff << 1) | 1ull, BufferPtr); \
	FTraceUtils::Encode7bit(SpecId, BufferPtr); \
	ThreadBuffer->BufferSize = (uint16_t)(BufferPtr - ThreadBuffer->Buffer); \
	if (ThreadBuffer->BufferSize >= FCpuProfilerTraceInternal::FullBufferThreshold) \
	{ \
		FCpuProfilerTraceInternal::FlushThreadBuffer(ThreadBuffer); \
	}

uint32_t FCpuProfilerTraceInternal::GetNextSpecId()
{
    static std::atomic<uint32_t> NextSpecId;
    return (NextSpecId++) + 1;
}

uint32_t FCpuProfilerTrace::OutputEventType(const char* name, const char* file, uint32_t line)
{
    uint32_t SpecId = FCpuProfilerTraceInternal::GetNextSpecId();
    uint16_t NameLen = uint16_t(strlen(name));
    uint32_t DataSize = NameLen * sizeof(char);
//#if CPUPROFILERTRACE_FILE_AND_LINE_ENABLED
    uint16_t FileLen = (file != nullptr) ? uint16_t(strlen(file)) : 0;
    DataSize += FileLen * sizeof(char);
//#endif
    UE_TRACE_LOG(CpuProfiler, EventSpec, CpuChannel, DataSize)
        << EventSpec.Id(SpecId)
        << EventSpec.Name(name, NameLen)
//#if CPUPROFILERTRACE_FILE_AND_LINE_ENABLED
        << EventSpec.File(file, FileLen)
        << EventSpec.Line(line)
//#endif
        ;
    return SpecId;
}

uint32_t FCpuProfilerTrace::OutputEventType2(const char* name, int name_len, const char* file, int file_name_len, uint32_t line)
{
    uint32_t SpecId = FCpuProfilerTraceInternal::GetNextSpecId();
    uint16_t NameLen = uint16_t(name_len);
    uint32_t DataSize = NameLen * sizeof(char);
    //#if CPUPROFILERTRACE_FILE_AND_LINE_ENABLED
    uint16_t FileLen = (file != nullptr) ? uint16_t(file_name_len) : 0;
    DataSize += FileLen * sizeof(char);
    //#endif
    UE_TRACE_LOG(CpuProfiler, EventSpec, CpuChannel, DataSize)
        << EventSpec.Id(SpecId)
        << EventSpec.Name(name, NameLen)
        //#if CPUPROFILERTRACE_FILE_AND_LINE_ENABLED
        << EventSpec.File(file, FileLen)
        << EventSpec.Line(line)
        //#endif
        ;
    return SpecId;
}

void FCpuProfilerTrace::OutputBeginEvent(uint32_t SpecId)
{
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_PROLOGUE();
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_EPILOGUE();
}

void FCpuProfilerTrace::OutputBeginDynamicEvent(const char* name, const char* file, uint32_t line)
{
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_PROLOGUE();
    auto iter = ThreadBuffer->DynamicAnsiScopeNamesMap.find(name);
    uint32_t SpecId = 0;
    if (iter == ThreadBuffer->DynamicAnsiScopeNamesMap.end())
    {
        int32_t NameSize = (int)strlen(name) + 1;
        char* NameCopy = reinterpret_cast<char*>(ThreadBuffer->ChainAllocator.Allocate(NameSize, alignof(char)));
        memcpy(NameCopy, name, NameSize - 1);
        NameCopy[NameSize - 1] = 0;

        SpecId = OutputEventType(NameCopy, file, line);
        ThreadBuffer->DynamicAnsiScopeNamesMap.insert(NameCopy, SpecId);
    }
    else
    {
        SpecId = iter->second;
    }
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_EPILOGUE();
}

void FCpuProfilerTrace::OutputBeginDynamicEvent2(const char* name, int name_len, const char* file, int file_name_len, uint32_t line)
{
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_PROLOGUE();
    auto iter = ThreadBuffer->DynamicAnsiScopeNamesMap.find(name);
    uint32_t SpecId = 0;
    if (iter == ThreadBuffer->DynamicAnsiScopeNamesMap.end()) {
        int32_t NameSize = name_len + 1;
        char* NameCopy = reinterpret_cast<char*>(ThreadBuffer->ChainAllocator.Allocate(NameSize, alignof(char)));
        memcpy(NameCopy, name, NameSize - 1);
        NameCopy[NameSize - 1] = 0;
        SpecId = OutputEventType2(NameCopy, name_len, file, file_name_len, line);
        ThreadBuffer->DynamicAnsiScopeNamesMap.insert(NameCopy, SpecId);
    } else {
        SpecId = iter->second;
    }
    CPUPROFILERTRACE_OUTPUTBEGINEVENT_EPILOGUE();
}

void FCpuProfilerTrace::OutputEndEvent()
{
    --FCpuProfilerTraceInternal::ThreadDepth;
    FCpuProfilerTraceInternal::FThreadBuffer* ThreadBuffer = FCpuProfilerTraceInternal::ThreadBuffer;
    if (!ThreadBuffer)
    {
        ThreadBuffer = FCpuProfilerTraceInternal::CreateThreadBuffer();
    }
    uint64_t Cycle = UE::Trace::Private::TimeGetTimestamp();
    uint64_t CycleDiff = Cycle - ThreadBuffer->LastCycle;
    ThreadBuffer->LastCycle = Cycle;
    uint8_t* BufferPtr = ThreadBuffer->Buffer + ThreadBuffer->BufferSize;
    FTraceUtils::Encode7bit(CycleDiff << 1, BufferPtr);
    ThreadBuffer->BufferSize = (uint16_t)(BufferPtr - ThreadBuffer->Buffer);
    if ((FCpuProfilerTraceInternal::ThreadDepth == 0) | (ThreadBuffer->BufferSize >= FCpuProfilerTraceInternal::FullBufferThreshold))
    {
        FCpuProfilerTraceInternal::FlushThreadBuffer(ThreadBuffer);
    }
}

void FMiscTrace::OutputBeginFrame(ETraceFrameType FrameType)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(FrameChannel))
    {
        return;
    }

    uint64_t Cycle = UE::Trace::Private::TimeGetTimestamp();
    UE_TRACE_LOG(Misc, BeginFrame, FrameChannel)
        << BeginFrame.Cycle(Cycle)
        << BeginFrame.FrameType((uint8_t)FrameType);
}

void FMiscTrace::OutputEndFrame(ETraceFrameType FrameType)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(FrameChannel))
    {
        return;
    }

    uint64_t Cycle = UE::Trace::Private::TimeGetTimestamp();
    UE_TRACE_LOG(Misc, EndFrame, FrameChannel)
        << EndFrame.Cycle(Cycle)
        << EndFrame.FrameType((uint8_t)FrameType);
}

void FMiscTrace::OutputScreenshot(const core::string_ref& Name, uint64_t Cycle, uint32_t Width, uint32_t Height, UInt8* pngData, size_t pngSize)
{
	static std::atomic<UInt32> ScreenshotId(0);

	const UInt32 DataSize = (UInt32) pngSize;
	const UInt32 MaxChunkSize = 65535;
	UInt32 ChunkNum = (DataSize + MaxChunkSize - 1) / MaxChunkSize;

	UInt32 Id = ScreenshotId.fetch_add(1);
	UE_TRACE_LOG(Misc, ScreenshotHeader, ScreenshotChannel)
		<< ScreenshotHeader.Id(Id)
		<< ScreenshotHeader.Name(Name.data(), UInt16(Name.length()))
		<< ScreenshotHeader.Cycle(Cycle)
		<< ScreenshotHeader.Width(Width)
		<< ScreenshotHeader.Height(Height)
		<< ScreenshotHeader.TotalChunkNum(ChunkNum)
		<< ScreenshotHeader.Size(DataSize);

	UInt32 RemainingSize = DataSize;
	for (UInt32 Index = 0; Index < ChunkNum; ++Index)
	{
		UInt16 Size = (UInt16) (RemainingSize < MaxChunkSize ? RemainingSize: MaxChunkSize);
		const UInt8* ChunkData = pngData + MaxChunkSize * Index;
		UE_TRACE_LOG(Misc, ScreenshotChunk, ScreenshotChannel)
			<< ScreenshotChunk.Id(Id)
			<< ScreenshotChunk.ChunkNum(Index)
			<< ScreenshotChunk.Size(Size)
			<< ScreenshotChunk.Data(ChunkData, Size);

		RemainingSize -= Size;
	}

	Assert(RemainingSize == 0);
}

bool FMiscTrace::ShouldTraceScreenshot()
{
	return UE_TRACE_CHANNELEXPR_IS_ENABLED(ScreenshotChannel);
}

void FGpuProfilerTrace::BeginFrame(FGPUTimingCalibrationTimestamp& Calibration)
{
	if (!bool(GpuChannel))
	{
		return;
	}

	//GCurrentFrame.Calibration = Calibration;
	//ensure(GCurrentFrame.Calibration.CPUMicroseconds > 0 && GCurrentFrame.Calibration.GPUMicroseconds > 0);
	//GCurrentFrame.TimestampBase = 0;
	//GCurrentFrame.EventBufferSize = 0;
	//GCurrentFrame.bActive = true;

	//int32 NeededSize = CVarGpuProfilerMaxEventBufferSizeKB.GetValueOnRenderThread() * 1024;
	//if ((GCurrentFrame.MaxEventBufferSize != NeededSize) && (NeededSize > 0))
	//{
	//	FMemory::Free(GCurrentFrame.EventBuffer);
	//	GCurrentFrame.EventBuffer = (UInt8*)FMemory::Malloc(NeededSize);
	//	GCurrentFrame.MaxEventBufferSize = NeededSize;
	//}
}

void FGpuProfilerTrace::SpecifyEventByName(const core::string& Name)
{
	//if (!GCurrentFrame.bActive)
	//{
	//	return;
	//}

	// This function is only called from FRealtimeGPUProfilerFrame::UpdateStats
	// at the end of the frame, so the access to this container is thread safe

	//UInt32 Index = Name.GetComparisonIndex().ToUnstableInt();
	//if (!GEventNames.Contains(Index))
	//{
	//	GEventNames.Add(Index);

	//	FString String = Name.ToString();
	//	UInt32 NameLength = String.Len() + 1;
	//	static_assert(sizeof(TCHAR) == sizeof(UInt16), "");

	//	UE_TRACE_LOG(GpuProfiler, EventSpec, GpuChannel, NameLength * sizeof(UInt16))
	//		<< EventSpec.EventType(Index)
	//		<< EventSpec.Name((const UInt16*)(*String), NameLength);
	//}
}

void FGpuProfilerTrace::BeginEventByName(const core::string& Name, UInt32 FrameNumber, UInt64 TimestampMicroseconds)
{
}

void FGpuProfilerTrace::EndEvent(UInt64 TimestampMicroseconds)
{
}

void FGpuProfilerTrace::EndFrame(UInt32 GPUIndex)
{
	//if (GCurrentFrame.bActive && GCurrentFrame.EventBufferSize)
	{
		// This subtraction is intended to be performed on UInt64 to leverage the wrap around behavior defined by the standard
		//UInt64 Bias = GCurrentFrame.Calibration.CPUMicroseconds - GCurrentFrame.Calibration.GPUMicroseconds;

		//if (GPUIndex == 0)
		//{
		//	UE_TRACE_LOG(GpuProfiler, Frame, GpuChannel)
		//		<< Frame.CalibrationBias(Bias)
		//		<< Frame.TimestampBase(GCurrentFrame.TimestampBase)
		//		<< Frame.RenderingFrameNumber(GCurrentFrame.RenderingFrameNumber)
		//		<< Frame.Data(GCurrentFrame.EventBuffer, GCurrentFrame.EventBufferSize);
		//}
		//else if (GPUIndex == 1)
		//{
		//	UE_TRACE_LOG(GpuProfiler, Frame2, GpuChannel)
		//		<< Frame2.CalibrationBias(Bias)
		//		<< Frame2.TimestampBase(GCurrentFrame.TimestampBase)
		//		<< Frame2.RenderingFrameNumber(GCurrentFrame.RenderingFrameNumber)
		//		<< Frame2.Data(GCurrentFrame.EventBuffer, GCurrentFrame.EventBufferSize);
		//}
	}
}

void FGpuProfilerTrace::Deinitialize()
{
}

namespace
{
#if PLATFORMFILETRACE_DEBUG_ENABLED
    FCriticalSection OpenHandlesLock;
    TMap<UInt64, SInt32> OpenHandles;
#else
    std::atomic<int32_t> OpenFileHandleCount;
#endif
}

void FPlatformFileTrace::BeginOpen(const core::string_ref& Path)
{
    UE_TRACE_LOG(PlatformFile, BeginOpen, FileChannel)
        << BeginOpen.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << BeginOpen.Path(Path.data(), Path.length());
}

void FPlatformFileTrace::EndOpen(UInt64 FileHandle)
{
    UE_TRACE_LOG(PlatformFile, EndOpen, FileChannel)
        << EndOpen.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << EndOpen.FileHandle(FileHandle);
#if PLATFORMFILETRACE_DEBUG_ENABLED
    {
        FScopeLock OpenHandlesScopeLock(&OpenHandlesLock);
        ++OpenHandles.FindOrAdd(FileHandle, 0);
    }
#else
    ++OpenFileHandleCount;
#endif
}

void FPlatformFileTrace::FailOpen(const core::string_ref& Path)
{
    UE_TRACE_LOG(PlatformFile, EndOpen, FileChannel)
        << EndOpen.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << EndOpen.FileHandle(UInt64(-1));
}

void FPlatformFileTrace::BeginReOpen(UInt64 OldFileHandle)
{
#if UE_TRACE_ENABLED
    UE_TRACE_LOG(PlatformFile, BeginReOpen, FileChannel)
        << BeginReOpen.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << BeginReOpen.OldFileHandle(OldFileHandle);
#endif
}

void FPlatformFileTrace::EndReOpen(UInt64 NewFileHandle)
{
    UE_TRACE_LOG(PlatformFile, EndReOpen, FileChannel)
        << EndReOpen.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << EndReOpen.NewFileHandle(NewFileHandle);
#if PLATFORMFILETRACE_DEBUG_ENABLED
    {
        FScopeLock OpenHandlesScopeLock(&OpenHandlesLock);
        ++OpenHandles.FindOrAdd(NewFileHandle, 0);
    }
#else
    ++OpenFileHandleCount;
#endif
}

void FPlatformFileTrace::BeginClose(UInt64 FileHandle)
{
    UE_TRACE_LOG(PlatformFile, BeginClose, FileChannel)
        << BeginClose.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << BeginClose.FileHandle(FileHandle);
}

void FPlatformFileTrace::EndClose(UInt64 FileHandle)
{
    UE_TRACE_LOG(PlatformFile, EndClose, FileChannel)
        << EndClose.Cycle(UE::Trace::Private::TimeGetTimestamp());
#if PLATFORMFILETRACE_DEBUG_ENABLED
    bool bUnderflow = false;
    {
        FScopeLock OpenHandlesScopeLock(&OpenHandlesLock);
        SInt32& OpenCount = OpenHandles.FindOrAdd(FileHandle, 0);
        --OpenCount;
        if (OpenCount <= 0)
        {
            bUnderflow = OpenCount < 0;
            OpenHandles.Remove(FileHandle);
        }
    }
    if (bUnderflow)
    {
        UE_LOG(LogCore, Error, TEXT("FPlatformFileTrace Close without an Open: FileHandle %llu."), FileHandle);
    }
#else
    int32_t NewValue = --OpenFileHandleCount;
    if (NewValue == -1)
    {
        //UE_LOG(LogCore, Error, TEXT("FPlatformFileTrace Close without an Open"));
        ++OpenFileHandleCount; // clamp the value to 0
    }
#endif
}

void FPlatformFileTrace::FailClose(UInt64 FileHandle)
{
    UE_TRACE_LOG(PlatformFile, EndClose, FileChannel)
        << EndClose.Cycle(UE::Trace::Private::TimeGetTimestamp());
}

void FPlatformFileTrace::BeginRead(UInt64 ReadHandle, UInt64 FileHandle, UInt64 Offset, UInt64 Size)
{
    UE_TRACE_LOG(PlatformFile, BeginRead, FileChannel)
        << BeginRead.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << BeginRead.ReadHandle(ReadHandle)
        << BeginRead.FileHandle(FileHandle)
        << BeginRead.Offset(Offset)
        << BeginRead.Size(Size);
}

void FPlatformFileTrace::EndRead(UInt64 ReadHandle, UInt64 SizeRead)
{
    UE_TRACE_LOG(PlatformFile, EndRead, FileChannel)
        << EndRead.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << EndRead.ReadHandle(ReadHandle)
        << EndRead.SizeRead(SizeRead);
}

void FPlatformFileTrace::BeginWrite(UInt64 WriteHandle, UInt64 FileHandle, UInt64 Offset, UInt64 Size)
{
    UE_TRACE_LOG(PlatformFile, BeginWrite, FileChannel)
        << BeginWrite.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << BeginWrite.WriteHandle(WriteHandle)
        << BeginWrite.FileHandle(FileHandle)
        << BeginWrite.Offset(Offset)
        << BeginWrite.Size(Size);
}

void FPlatformFileTrace::EndWrite(UInt64 WriteHandle, UInt64 SizeWritten)
{
    UE_TRACE_LOG(PlatformFile, EndWrite, FileChannel)
        << EndWrite.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << EndWrite.WriteHandle(WriteHandle)
        << EndWrite.SizeWritten(SizeWritten);
}

UInt32 FPlatformFileTrace::GetOpenFileHandleCount()
{
#if PLATFORMFILETRACE_DEBUG_ENABLED
    return OpenHandles.Num();
#else
    return OpenFileHandleCount;
#endif
}

// Controls how often time markers are emitted (default every 4095 allocation)
constexpr UInt32 MarkerSamplePeriod = (4 << 10) - 1;
// Number of bits shifted bits to SizeLower
constexpr UInt32 SizeShift = 3;
std::atomic<UInt32>	GMarkerCounter(0);
bool				GTraceAllowed = false;

// If enabled also pumps the Trace system itself. Used on process shutdown
// when worker thread has been killed, but memory events still occurs.
bool				GDoPumpTrace = false;

static FORCEINLINE UInt32 CountTrailingZeros(UInt32 Value)
{
#if _MSC_VER
    // return 32 if value was 0
    unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 31
    return _BitScanForward(&BitIndex, Value) ? BitIndex : 32;
#else
    if (Value == 0)
    {
        return 32;
    }

    return (UInt32)__builtin_ctz(Value);
#endif
}
#endif // Trace Enabled

enum
{
    // Default allocator alignment. If the default is specified, the allocator applies to engine rules.
    // Blocks >= 16 bytes will be 16-byte-aligned, Blocks < 16 will be 8-byte aligned. If the allocator does
    // not support allocation alignment, the alignment will be ignored.
    DEFAULT_ALIGNMENT = 0,

    // Minimum allocator alignment
    MIN_ALIGNMENT = 8,
};

namespace UE
{
namespace Trace
{
    TRACELOG_API void Update();
} // namespace Trace
} // namespace UE

typedef UInt32 HeapId;

HeapId engineHeapId = 0;
HeapId libcHeapId = 0;
HeapId programHeapId = 0;

void utrace_mem_update_internal();

void utrace_mem_init()
{
#if UE_TRACE_ENABLED
    if (!TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(MemAllocChannel))
    {
        return;
    }

    if (GTraceAllowed)
        return;

    atexit([]() { GDoPumpTrace = true; });

    GTraceAllowed = true;

    UE_TRACE_LOG(Memory, Init, MemAllocChannel)
        << Init.MarkerPeriod(MarkerSamplePeriod + 1)
        << Init.Version(MemoryTraceVersion)
        << Init.MinAlignment(UInt8(MIN_ALIGNMENT))
        << Init.SizeShift(UInt8(SizeShift));
    // Engine Allocated
    const HeapId SystemRootHeap = utrace_mem_root_heap_spec(MAKE_UTRACE_STR("System memory"), (UInt32)EMemoryTraceHeapFlags::None);
    check(SystemRootHeap == EMemoryTraceRootHeap::SystemMemory);
    // GPU Driver allocated
    const HeapId VideoRootHeap = utrace_mem_root_heap_spec(MAKE_UTRACE_STR("Video memory"), (UInt32)EMemoryTraceHeapFlags::None);
    check(VideoRootHeap == EMemoryTraceRootHeap::VideoMemory);
    engineHeapId = utrace_mem_root_heap_spec(MAKE_UTRACE_STR("Engine memory"), (UInt32)EMemoryTraceHeapFlags::None);
    libcHeapId = utrace_mem_root_heap_spec(MAKE_UTRACE_STR("libc memory"), (UInt32)EMemoryTraceHeapFlags::None);
    programHeapId = utrace_mem_root_heap_spec(MAKE_UTRACE_STR("Program"), (UInt32)EMemoryTraceHeapFlags::None);
    static_assert((1 << SizeShift) - 1 <= MIN_ALIGNMENT, "Not enough bits to pack size fields");
#endif
}

FORCEINLINE UInt32 GetCallstackId()
{
    if (!GCaptureCallStack)
        return 0;
    using namespace UE::Trace;
    static thread_local void *buffer[CALLSTACK_BUFFERSIZE];
    UInt32 hash = 0;
    auto numStacks = (UInt32)UE::Trace::Private::GetCallbackFrames(buffer, CALLSTACK_BUFFERSIZE, 0, hash,
                                                           GUseFastCallStackTrace);
    void *AddressOfReturnAddress = PLATFORM_RETURN_ADDRESS_POINTER();
    if (FBacktracer *Instance = FBacktracer::Get()) {
        return Instance->GetBacktraceId(hash, numStacks, buffer);
    }
    return 0;
}

void utrace_mem_alloc_with_callstack(UInt64 Address, UInt64 Size, UInt32 Alignment, utrace_u32 rootHeap, utrace_u32 callstackId)
{
    if (!GTraceAllowed || Address == 0 || Size == 0)
    {
        return;
    }
    const UInt32 AlignmentPow2 = UInt32(CountTrailingZeros(Alignment));
    const UInt32 Alignment_SizeLower = (AlignmentPow2 << SizeShift) | UInt32(Size & ((1 << SizeShift) - 1));
    switch (rootHeap)
    {
        case EMemoryTraceRootHeap::SystemMemory:
        {
            UE_TRACE_LOG(Memory, AllocSystem, MemAllocChannel)
                                        << AllocSystem.Address(UInt64(Address))
                                        << AllocSystem.CallstackId(callstackId)
                                        << AllocSystem.Size(UInt32(Size >> SizeShift))
                                        << AllocSystem.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower));
            break;
        }

        case EMemoryTraceRootHeap::VideoMemory:
        {
            UE_TRACE_LOG(Memory, AllocVideo, MemAllocChannel)
                                        << AllocVideo.Address(UInt64(Address))
                                        << AllocVideo.CallstackId(callstackId)
                                        << AllocVideo.Size(UInt32(Size >> SizeShift))
                                        << AllocVideo.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower));
            break;
        }

        default:
        {
            UE_TRACE_LOG(Memory, Alloc, MemAllocChannel)
                                        << Alloc.Address(UInt64(Address))
                                        << Alloc.CallstackId(callstackId)
                                        << Alloc.Size(UInt32(Size >> SizeShift))
                                        << Alloc.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower))
                                        << Alloc.RootHeap(UInt8(rootHeap));
            break;
        }
    }
    utrace_mem_update_internal();
}

utrace_u32 utrace_mem_alloc(UInt64 Address, UInt64 Size, UInt32 Alignment, utrace_u32 rootHeap)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed || Address == 0 || Size == 0)
    {
        return 0;
    }
    const UInt32 CallstackId = GetCallstackId(); // TODO: GDoNotAllocateInTrace ? 0 : CallstackTrace_GetCurrentId();
    utrace_mem_alloc_with_callstack(Address, Size, Alignment, rootHeap, CallstackId);
    return CallstackId;
#endif
}

utrace_u32 utrace_mem_free(UInt64 Address, utrace_u32 rootHeap)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed || Address == 0)
    {
        return 0;
    }

    const UInt32 CallstackId = GetCallstackId();

    switch (rootHeap) 
    {
    case EMemoryTraceRootHeap::SystemMemory:
    {
        UE_TRACE_LOG(Memory, FreeSystem, MemAllocChannel)
            << FreeSystem.Address(UInt64(Address))
            << FreeSystem.CallstackId(CallstackId);
        break;
    }
    case EMemoryTraceRootHeap::VideoMemory:
    {
        UE_TRACE_LOG(Memory, FreeVideo, MemAllocChannel)
            << FreeVideo.Address(UInt64(Address))
            << FreeVideo.CallstackId(CallstackId);
        break;
    }
    default: 
    {
        UE_TRACE_LOG(Memory, Free, MemAllocChannel)
            << Free.Address(UInt64(Address))
            << Free.CallstackId(CallstackId)
            << Free.RootHeap(UInt8(rootHeap));
        break;
    }
    }
    utrace_mem_update_internal();
    return CallstackId;
#endif
}

void utrace_mem_realloc_free(UInt64 Address, utrace_u32 rootHeap)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }

    const UInt32 CallstackId = GetCallstackId();
        //GDoNotAllocateInTrace ? 0 : CallstackTrace_GetCurrentId();

    switch (rootHeap) 
    {
    case EMemoryTraceRootHeap::SystemMemory: 
    {
        UE_TRACE_LOG(Memory, ReallocFreeSystem, MemAllocChannel)
            << ReallocFreeSystem.Address(UInt64(Address))
            << ReallocFreeSystem.CallstackId(CallstackId);
        break;
    }

    default: 
    {
        UE_TRACE_LOG(Memory, ReallocFree, MemAllocChannel)
            << ReallocFree.Address(UInt64(Address))
            << ReallocFree.CallstackId(CallstackId)
            << ReallocFree.RootHeap(UInt8(rootHeap));
        break;
    }
    }

    utrace_mem_update_internal();
#endif
}

void utrace_mem_realloc_alloc(UInt64 Address, UInt64 NewSize, UInt32 Alignment, utrace_u32 rootHeap)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }
    const UInt32 AlignmentPow2 = UInt32(CountTrailingZeros(Alignment));
    const UInt32 Alignment_SizeLower = (AlignmentPow2 << SizeShift) | UInt32(NewSize & ((1 << SizeShift) - 1));
    const UInt32 CallstackId = GetCallstackId(); //GDoNotAllocateInTrace ? 0 : CallstackTrace_GetCurrentId();
    switch (rootHeap) 
    {
    case EMemoryTraceRootHeap::SystemMemory: 
    {
        UE_TRACE_LOG(Memory, ReallocAllocSystem, MemAllocChannel)
            << ReallocAllocSystem.Address(UInt64(Address))
            << ReallocAllocSystem.CallstackId(CallstackId)
            << ReallocAllocSystem.Size(UInt32(NewSize >> SizeShift))
            << ReallocAllocSystem.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower));
        break;
    }
    default:
    {
        UE_TRACE_LOG(Memory, ReallocAlloc, MemAllocChannel)
            << ReallocAlloc.Address(UInt64(Address))
            << ReallocAlloc.CallstackId(CallstackId)
            << ReallocAlloc.Size(UInt32(NewSize >> SizeShift))
            << ReallocAlloc.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower))
            << ReallocAlloc.RootHeap(UInt8(rootHeap));
        break;
    }
    }

    utrace_mem_update_internal();
#endif
}

void utrace_mem_update_internal()
{
#if UE_TRACE_ENABLED
    const UInt32 TheCount = GMarkerCounter.fetch_add(1, std::memory_order_relaxed);
    if ((TheCount & MarkerSamplePeriod) == 0)
    {
        UE_TRACE_LOG(Memory, Marker, MemAllocChannel)
            << Marker.Cycle(UE::Trace::Private::TimeGetTimestamp());
    }

    if (GDoPumpTrace)
    {
        UE::Trace::Update();
    }
#endif
}

constexpr SInt32 TRACE_TAG = 257;
static thread_local SInt32 GActiveTag = 0;

class FTagTrace
{
public:
    FTagTrace();

    void			AnnounceGenericTags() const;
    void 			AnnounceTagDeclarations();
    void            AnnounceSpecialTags() const;
    SInt32			AnnounceCustomTag(SInt32 Tag, SInt32 ParentTag, const utrace_string& Display) const;
    SInt32 			AnnounceFNameTag(const utrace_string& TagName);

private:

    static constexpr SInt32 FNAME_INDEX_OFFSET = 512;

    struct FTagNameSetEntry
    {
        std::atomic_int32_t Data;

        SInt32 GetKey() const { return Data.load(std::memory_order_relaxed); }
        bool GetValue() const { return true; }
        bool IsEmpty() const { return Data.load(std::memory_order_relaxed) == 0; }			// NAME_None is treated as empty
        void SetKeyValue(SInt32 Key, bool Value) { Data.store(Key, std::memory_order_relaxed); }
        static UInt32 KeyHash(SInt32 Key) { return static_cast<UInt32>(Key); }
        static void ClearEntries(FTagNameSetEntry* Entries, SInt32 EntryCount)
        {
            memset(Entries, 0, EntryCount * sizeof(FTagNameSetEntry));
        }
    };
    typedef TGrowOnlyLockFreeHash<FTagNameSetEntry, UInt32, bool> FTagNameSet;

    FTagNameSet				AnnouncedNames;
};

static FTagTrace* GTagTrace = nullptr;
////////////////////////////////////////////////////////////////////////////////
FTagTrace::FTagTrace()
{
    AnnouncedNames.Reserve(1024);
    AnnounceGenericTags();
    AnnounceTagDeclarations();
    AnnounceSpecialTags();
}

////////////////////////////////////////////////////////////////////////////////
void FTagTrace::AnnounceGenericTags() const
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER
#define TRACE_TAG_SPEC(Enum,Str,Stat,Group,ParentTag)\
	{\
		const UInt32 DisplayLen = FCStringAnsi::Strlen(Str);\
		UE_TRACE_LOG(Memory, TagSpec, MemAllocChannel, DisplayLen * sizeof(ANSICHAR))\
			<< TagSpec.Tag((SInt32) ELLMTag::Enum)\
			<< TagSpec.Parent((SInt32) ParentTag)\
			<< TagSpec.Display(Str, DisplayLen);\
	}
    LLM_ENUM_GENERIC_TAGS(TRACE_TAG_SPEC);
#undef TRACE_TAG_SPEC
#endif
}

////////////////////////////////////////////////////////////////////////////////
void FTagTrace::AnnounceTagDeclarations()
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER
    FLLMTagDeclaration* List = FLLMTagDeclaration::GetList();
    while (List)
    {
        OnAnnounceTagDeclaration(*List);
        List = List->Next;
    }
    FLLMTagDeclaration::AddCreationCallback(FTagTrace::OnAnnounceTagDeclaration);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void FTagTrace::AnnounceSpecialTags() const
{
    auto EmitTag = [](const char* DisplayString, SInt32 Tag, SInt32 ParentTag)
    {
        const SInt32 DisplayLen = 0;//FCString::Strlen(DisplayString);
        UE_TRACE_LOG(Memory, TagSpec, MemAllocChannel, DisplayLen)
            << TagSpec.Tag(Tag)
            << TagSpec.Parent(ParentTag)
            << TagSpec.Display(DisplayString, DisplayLen);
    };

    //EmitTag(TEXT("Trace"), TRACE_TAG, -1);
}

////////////////////////////////////////////////////////////////////////////////
SInt32 FTagTrace::AnnounceFNameTag(const utrace_string& Name)
{
#if 1
    auto hash = [](const char *str) -> UInt32
    {
        UInt32 hash = 5381;
        int c;

        while (c = (utrace_u8)(*str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    };

    const auto NameIndex = hash(Name.str);
    // Find or add the item
    bool bAlreadyInTable;
    AnnouncedNames.FindOrAdd(NameIndex, true, &bAlreadyInTable);
    if (bAlreadyInTable)
    {
        return NameIndex;
    }
    // First time encountering this name, announce it
    //ANSICHAR NameString[NAME_SIZE];
    //Name.GetPlainANSIString(NameString);
    SInt32 ParentTag = -1;
    std::string_view NameView(Name.str, Name.len);
    auto LeafStart = NameView.find_first_of('/');
    if (LeafStart != std::string::npos)
    {
        std::string_view ParentName(NameView.substr(LeafStart));
        ParentTag = AnnounceFNameTag({ ParentName.data(), (utrace_u32)ParentName.size() });
    }
    return AnnounceCustomTag(NameIndex, ParentTag, Name);
#else
    return 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
SInt32 FTagTrace::AnnounceCustomTag(SInt32 Tag, SInt32 ParentTag, const utrace_string& Display) const
{
    UE_TRACE_LOG(Memory, TagSpec, MemAllocChannel, Display.len)
        << TagSpec.Tag(Tag)
        << TagSpec.Parent(ParentTag)
        << TagSpec.Display(Display.str, Display.len);
    return Tag;
}


FMemScope::FMemScope(SInt32 InTag, bool bShouldActivate /*= true*/)
{
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate)
    {
        ActivateScope(InTag);
    }
}

////////////////////////////////////////////////////////////////////////////////
FMemScope::FMemScope(const utrace_string& InName, bool bShouldActivate /*= true*/)
{
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate)
    {
        ActivateScope(utrace_mem_name_tag(InName));
    }
}

////////////////////////////////////////////////////////////////////////////////
void FMemScope::ActivateScope(SInt32 InTag)
{
    if (auto LogScope = FMemoryMemoryScopeFields::LogScopeType::ScopedEnter<FMemoryMemoryScopeFields>())
    {
        if (const auto& __restrict MemoryScope = *(FMemoryMemoryScopeFields*)(&LogScope))
        {
            Inner.SetActive();
            LogScope += LogScope << MemoryScope.Tag(InTag);
            PrevTag = GActiveTag;
            GActiveTag = InTag;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
FMemScope::~FMemScope()
{
    if (Inner.bActive)
    {
        GActiveTag = PrevTag;
    }
}

////////////////////////////////////////////////////////////////////////////////
FMemScopePtr::FMemScopePtr(UInt64 InPtr)
{
    if (InPtr != 0 && TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(MemAllocChannel))
    {
        if (auto LogScope = FMemoryMemoryScopePtrFields::LogScopeType::ScopedEnter<FMemoryMemoryScopePtrFields>())
        {
            if (const auto& __restrict MemoryScope = *(FMemoryMemoryScopePtrFields*)(&LogScope))
            {
                Inner.SetActive(), LogScope += LogScope << MemoryScope.Ptr(InPtr);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
FMemScopePtr::~FMemScopePtr()
{
}

SInt32 utrace_mem_announce_custom_tag(SInt32 Tag, SInt32 ParentTag, utrace_string Display)
{
#if UE_TRACE_ENABLED
    // todo: How do we check if tag trace is active?
    if (GTagTrace) {
        return GTagTrace->AnnounceCustomTag(Tag, ParentTag, Display);
    }
#endif
    return -1;
}

void utrace_mem_init_tags()
{
    if (!GTagTrace)
    {
        GTagTrace = (FTagTrace*)malloc(sizeof(FTagTrace) /*, alignof(FTagTrace)*/);
        new (GTagTrace) FTagTrace;
    }
}

SInt32 utrace_mem_name_tag(utrace_string TagName)
{
#if UE_TRACE_ENABLED
    if (GTagTrace) 
    {
        return GTagTrace->AnnounceFNameTag(TagName);
    }
#endif
    return -1;
}

SInt32 utrace_mem_get_active_tag()
{
#if UE_TRACE_ENABLED
    return GActiveTag;
#else
    return -1;
#endif
}

void utrace_mem_mark_object_name(void* address, utrace_string objName)
{
    if (!GTraceAllowed) {
        return;
    }
    
    UE_TRACE_LOG(Memory, MarkObject, MemAllocChannel)
        << MarkObject.Address(UInt64(address))
        << MarkObject.Name(objName.str, objName.len);
}

utrace_u32 utrace_mem_heap_spec(utrace_u32 parentId, utrace_string name, utrace_u32 flags)
{
    if (!GTraceAllowed) {
        return 0;
    }

    static std::atomic<UInt32> HeapIdCount(EMemoryTraceRootHeap::EndReserved + 1); // Reserve indexes for root heaps
    const UInt32 Id = HeapIdCount.fetch_add(1);
    const UInt32 NameLen = name.len;
    const UInt32 DataSize = NameLen;
    check(parentId < Id);

    UE_TRACE_LOG(Memory, HeapSpec, MemAllocChannel, DataSize)
        << HeapSpec.Id(Id)
        << HeapSpec.ParentId(parentId)
        << HeapSpec.Name(name.str, NameLen)
        << HeapSpec.Flags(UInt16(flags));

    return Id;
}

utrace_u32 utrace_mem_root_heap_spec(utrace_string name, utrace_u32 flags)
{
    if (!GTraceAllowed) {
        return 0;
    }

    static std::atomic<UInt32> RootHeapCount(0);
    const UInt32 Id = RootHeapCount.fetch_add(1);
    check(Id <= EMemoryTraceRootHeap::EndReserved);

    const UInt32 DataSize = name.len;

    UE_TRACE_LOG(Memory, HeapSpec, MemAllocChannel, DataSize)
        << HeapSpec.Id(Id)
        << HeapSpec.ParentId(UInt32(~0))
        << HeapSpec.Name(name.str, name.len)
        << HeapSpec.Flags(UInt16(UInt32(EMemoryTraceHeapFlags::Root) | flags));

    return Id;
}


FORCEINLINE void utrace_mem_mark_alloc_as_heap_with_callstack(utrace_u64 addr, utrace_u32 heap, utrace_u32 flags, UInt32 callstackId)
{
    if (!GTraceAllowed) {
        return;
    }
    UE_TRACE_LOG(Memory, HeapMarkAlloc, MemAllocChannel)
                                << HeapMarkAlloc.Address(UInt64(addr))
                                << HeapMarkAlloc.CallstackId(callstackId)
                                << HeapMarkAlloc.Flags(UInt16(UInt16(EMemoryTraceHeapAllocationFlags::Heap) | flags))
                                << HeapMarkAlloc.Heap(heap);
}

void utrace_mem_mark_alloc_as_heap(utrace_u64 addr, utrace_u32 heap, utrace_u32 flags)
{
    if (!GTraceAllowed) {
        return;
    }
    const UInt32 callstackId = GetCallstackId();
    utrace_mem_mark_alloc_as_heap_with_callstack(addr, heap, flags, callstackId);
}

void utrace_mem_unmark_alloc_as_heap_with_callstack(utrace_u64 addr, utrace_u32 heap, utrace_u32 callStackId)
{
    if (!GTraceAllowed) {
        return;
    }
    // Sets all flags to zero
    UE_TRACE_LOG(Memory, HeapUnmarkAlloc, MemAllocChannel)
                                << HeapUnmarkAlloc.Address(UInt64(addr))
                                << HeapUnmarkAlloc.CallstackId(callStackId)
                                << HeapUnmarkAlloc.Heap(heap);
}

void utrace_mem_unmark_alloc_as_heap(utrace_u64 addr, utrace_u32 heap)
{
    if (!GTraceAllowed) {
        return;
    }

    const UInt32 callstackId = GetCallstackId();
    utrace_mem_unmark_alloc_as_heap_with_callstack(addr, heap, callstackId);
}

void utrace_mem_scope_ctor(void* ptr, utrace_string name, utrace_u8 active)
{
    new (ptr) FMemScope(name, active);
}

void utrace_mem_scope_dtor(void* ptr)
{
    FMemScope* scope = (FMemScope*)ptr;
    scope->~FMemScope();
}

void utrace_mem_scope_ptr_ctor(void* ptr, void* addr)
{
    new (ptr) FMemScopePtr((UInt64)addr);
}

void utrace_mem_scope_ptr_dtor(void* ptr)
{
    FMemScopePtr* p = (FMemScopePtr*)ptr;
    p->~FMemScopePtr();
}

void utrace_vulkan_set_render_pass_name(utrace_u64 render_pass, utrace_string name)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(VulkanChannel))
        return;
    UE_TRACE_LOG(Vulkan, SetRenderPassName, VulkanChannel)
        << SetRenderPassName.RenderPass(render_pass)
        << SetRenderPassName.Name(name.str, name.len);
}

void utrace_vulkan_set_frame_buffer_name(utrace_u64 frame_buffer, utrace_string name)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(VulkanChannel))
        return;
    UE_TRACE_LOG(Vulkan, SetFrameBufferName, VulkanChannel)
        << SetFrameBufferName.FrameBuffer(frame_buffer)
        << SetFrameBufferName.Name(name.str, name.len);
}

void utrace_vulkan_set_command_buffer_name(utrace_u64 command_buffer, utrace_string name)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(VulkanChannel))
        return;
    UE_TRACE_LOG(Vulkan, SetCommandBufferName, VulkanChannel)
        << SetCommandBufferName.CommandBuffer(command_buffer)
        << SetCommandBufferName.Name(name.str, name.len);
}

////////////////////////////////////////////////////////////////////////////////
enum class ETraceConnectType
{
    Network,
    File
};

////////////////////////////////////////////////////////////////////////////////
class FTraceAuxiliaryImpl
{
public:
    const char* GetDest() const;
    bool					IsConnected() const;
    void					AddCommandlineChannels(const char* ChannelList);
    void					ResetCommandlineChannels();
    bool					HasCommandlineChannels() const { return !CommandlineChannels.empty(); };
    void					EnableChannels(const char* ChannelList);
    void					DisableChannels(const char* ChannelList);
    bool					Connect(ETraceConnectType Type, const char* Parameter);
    bool					Stop();
    void					ResumeChannels();
    void					PauseChannels();
    void					EnableCommandlineChannels();
    void					SetTruncateFile(bool bTruncateFile);
    void					StartWorkerThread();
    void					StartEndFramePump();
    void                    StartGpuTrace();
    void                    InjectSysMemAlloc();

#if PLATFORM_ANDROID
    static void             OnDlNotify(int flag, const char* so_name, const char* realpath, ElfW(Addr) base, const ElfW(Phdr)* phdr, ElfW(Half) phnum, void*);
    static int              OnDlIterate(struct dl_phdr_info* info, size_t size, void* data);
#endif
private:
    enum class EState : UInt8
    {
        Stopped,
        Tracing,
    };

    struct FChannelEntry
    {
        core::string		Name;
        bool				bActive = false;
    };

    void					AddChannel(const char* Name);
    void					RemoveChannel(const char* Name);
    template <class T> void ForEachChannel(const char* ChannelList, bool bResolvePresets, T Callable);
    static UInt32			HashChannelName(const char* Name);
    bool					EnableChannel(const char* Channel);
    void					DisableChannel(const char* Channel);
    bool					SendToHost(const char* Host);
    bool					WriteToFile(const char* Path);

    typedef core::hash_map<UInt32, FChannelEntry> ChannelSet;
    ChannelSet				CommandlineChannels;
    core::string			TraceDest;
    EState					State = EState::Stopped;
    bool					bTruncateFile = false;
    bool					bWorkerThreadStarted = false;
    core::string			PausedPreset;
    class PerfettoCmd*      Perfetto = nullptr;
    Thread                  GpuDataThread;
    std::atomic_bool        bPerfettoStopped;
};

static FTraceAuxiliaryImpl GTraceAuxiliary;

const char* GDefaultChannels = "cpu,gpu,frame,log,file,counters,bookmark"; // TODO: memalloc,
const char* GMemoryChannels = "module,callstack,memalloc"; // memory channels

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::AddCommandlineChannels(const char* ChannelList)
{
    ForEachChannel(ChannelList, true, &FTraceAuxiliaryImpl::AddChannel);
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::ResetCommandlineChannels()
{
    CommandlineChannels.clear();
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::EnableChannels(const char* ChannelList)
{
    if (ChannelList)
    {
        ForEachChannel(ChannelList, true, &FTraceAuxiliaryImpl::EnableChannel);
    }
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::DisableChannels(const char* ChannelList)
{
    if (ChannelList)
    {
        ForEachChannel(ChannelList, true, &FTraceAuxiliaryImpl::DisableChannel);
    }
    else
    {
#if 0
        // Disable all channels.
        TStringBuilder<128> EnabledChannels;
        GetActiveChannelsString(EnabledChannels);
        ForEachChannel(EnabledChannels.ToString(), true, &FTraceAuxiliaryImpl::DisableChannel);
#endif
    }
}

template <typename T>
void ParseTokens(const core::string& instr, char delim, T callable)
{
    size_t start = 0U;
    size_t end = instr.find(delim);
    while (end != std::string::npos)
    {
        core::string_ref piece(instr.c_str() + start, end - start);
        callable(piece);
        start = end + 1;
        end = instr.find(delim, start);
    }
    size_t length = instr.length() - start;
    core::string_ref piece(instr.c_str() + start, length);
    callable(piece);
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
void FTraceAuxiliaryImpl::ForEachChannel(const char* ChannelList, bool bResolvePresets, T Callable)
{
    Assert(ChannelList);
    ParseTokens(ChannelList, ',', [this, bResolvePresets, Callable](const core::string_ref& Token)
        {
            if (bResolvePresets)
            {
                core::string Value;
                // Check against hard coded presets
                if (Token.equals("default", false))
                {
                    ForEachChannel(GDefaultChannels, false, Callable);
                    return;
                }
                else if (Token.equals("memory", false))
                {
                    ForEachChannel(GMemoryChannels, false, Callable);
                }
            }
            auto name = (core::string)Token;
            (this->*Callable)(name.c_str());
        });
}

////////////////////////////////////////////////////////////////////////////////
UInt32 FTraceAuxiliaryImpl::HashChannelName(const char* Name)
{
    UInt32 Hash = 5381;
    for (const char* c = Name; *c; ++c)
    {
        UInt32 LowerC = *c | 0x20;
        Hash = ((Hash << 5) + Hash) + LowerC;
    }
    return Hash;
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::AddChannel(const char* Name)
{
    UInt32 Hash = HashChannelName(Name);

    if (CommandlineChannels.find(Hash) != CommandlineChannels.end())
    {
        return;
    }
    FChannelEntry& Value = CommandlineChannels[Hash];
    Value.Name = Name;

    if (State >= EState::Tracing && !Value.bActive)
    {
        Value.bActive = EnableChannel(*Value.Name);
    }
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::RemoveChannel(const char* Name)
{
    UInt32 Hash = HashChannelName(Name);

    FChannelEntry Channel;
#if 0
    if (!CommandlineChannels.RemoveAndCopyValue(Hash, Channel))
    {
        return;
    }
#endif
    if (State >= EState::Tracing && Channel.bActive)
    {
        DisableChannel(*Channel.Name);
        Channel.bActive = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::Connect(ETraceConnectType Type, const char* Parameter)
{
    // Connect/write to file, but only if we're not already sending/writing.
    bool bConnected = UE::Trace::IsTracing();
    if (!bConnected)
    {
        if (Type == ETraceConnectType::Network)
        {
            bConnected = SendToHost(Parameter);
            if (bConnected)
            {
                LogStringMsg("Trace started (connected to trace server %s).", GetDest());
            }
            else
            {
                ErrorStringMsg("Trace failed to connect (trace server: %s)!", Parameter ? Parameter : "");
            }
        }

        else if (Type == ETraceConnectType::File)
        {
            bConnected = WriteToFile(Parameter);
            if (bConnected)
            {
                LogStringMsg("Trace started (writing to file \"%s\").", GetDest());
            }
            else
            {
                ErrorStringMsg("Trace failed to connect (file: \"%s\")!", Parameter ? Parameter : "");
            }
        }
    }

    if (!bConnected)
    {
        return false;
    }

    State = EState::Tracing;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::Stop()
{
    if (!UE::Trace::Stop())
    {
        return false;
    }
    bPerfettoStopped.store(true);
    State = EState::Stopped;
    TraceDest.clear();

    GpuDataThread.WaitForExit();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::EnableChannel(const char* Channel)
{
    // Channel names have been provided by the user and may not exist yet. As
    // we want to maintain bActive accurately (channels toggles are reference
    // counted), we will first check Trace knows of the channel.
    if (!UE::Trace::IsChannel(Channel))
    {
        return false;
    }
#if 0
    EPlatformEvent Event = PlatformEvents_GetEvent(Channel);
    if (Event != EPlatformEvent::None)
    {
        PlatformEvents_Enable(Event);
    }
#endif
    return UE::Trace::ToggleChannel(Channel, true);
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::DisableChannel(const char* Channel)
{
#if 0
    // Channel names have been provided by the user and may not exist yet. As
    // we want to maintain bActive accurately (channels toggles are reference
    // counted), we will first check Trace knows of the channel.
    if (!UE::Trace::IsChannel(Channel))
    {
        return;
    }

    EPlatformEvent Event = PlatformEvents_GetEvent(Channel);
    if (Event != EPlatformEvent::None)
    {
        PlatformEvents_Disable(Event);
    }

    UE::Trace::ToggleChannel(Channel, false);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::ResumeChannels()
{
    // Enable channels from the "paused" preset.
    //ForEachChannel(PausedPreset, false, &FTraceAuxiliaryImpl::EnableChannel);
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::PauseChannels()
{
#if 0
    TStringBuilder<128> EnabledChannels;
    GetActiveChannelsString(EnabledChannels);

    // Save the list of enabled channels as the current "paused" preset.
    // The "paused" preset can only be used in the Trace.Resume command / API.
    PausedPreset = EnabledChannels.ToString();

    // Disable all "paused" channels.
    ForEachChannel(*PausedPreset, true, &FTraceAuxiliaryImpl::DisableChannel);
#endif
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::EnableCommandlineChannels()
{
    for (auto& ChannelPair : CommandlineChannels)
    {
        if (!ChannelPair.second.bActive)
        {
            ChannelPair.second.bActive = EnableChannel(*ChannelPair.second.Name);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
void FTraceAuxiliaryImpl::SetTruncateFile(bool bNewTruncateFileState)
{
    bTruncateFile = bNewTruncateFileState;
}

void FTraceAuxiliaryImpl::StartWorkerThread()
{
    if (!bWorkerThreadStarted)
    {
        UE::Trace::StartWorkerThread();
        bWorkerThreadStarted = true;
    }
}

void FTraceAuxiliaryImpl::StartEndFramePump()
{
#if 0
    if (!GEndFrameDelegateHandle.IsValid())
    {
        // If the worker thread is disabled, pump the update from end frame
        GEndFrameDelegateHandle = FCoreDelegates::OnEndFrame.AddStatic(UE::Trace::Update);
    }
    if (!GEndFrameStatDelegateHandle.IsValid())
    {
        // Update stats every frame
        GEndFrameStatDelegateHandle = FCoreDelegates::OnEndFrame.AddLambda([]()
            {
                UE::Trace::FStatistics Stats;
                UE::Trace::GetStatistics(Stats);
                //SET_MEMORY_STAT(STAT_TraceMemoryUsed, Stats.MemoryUsed);
                //SET_MEMORY_STAT(STAT_TraceCacheUsed, Stats.CacheUsed);
                //SET_MEMORY_STAT(STAT_TraceCacheWaste, Stats.CacheWaste);
                //SET_MEMORY_STAT(STAT_TraceSent, Stats.BytesSent);
            });
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::SendToHost(const char* Host)
{
    if (!UE::Trace::SendTo(Host))
    {
        //UE_LOG_REF(LogCategory, Warning, TEXT("Unable to trace to host '%s'"), Host);
        return false;
    }

    TraceDest = Host;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::WriteToFile(const char* InPath)
{
#if 0
    const FStringView Path(InPath);

    // Default file name functor
    auto GetDefaultName = [] { return FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S.utrace")); };

    if (Path.IsEmpty())
    {
        const FString Name = GetDefaultName();
        return WriteToFile(*Name, LogCategory);
    }

    FString WritePath;
    // Relative paths go to the profiling directory
    if (FPathViews::IsRelativePath(Path))
    {
        WritePath = FPaths::Combine(FPaths::ProfilingDir(), InPath);
    }
#if PLATFORM_WINDOWS
    // On windows we treat paths starting with '/' as relative, except double slash which is a network path
    else if (FPathViews::IsSeparator(Path[0]) && !(Path.Len() > 1 && FPathViews::IsSeparator(Path[1])))
    {
        WritePath = FPaths::Combine(FPaths::ProfilingDir(), InPath);
    }
#endif
    else
    {
        WritePath = InPath;
    }

    // If a directory is specified, add the default trace file name
    if (FPathViews::GetCleanFilename(WritePath).IsEmpty())
    {
        WritePath = FPaths::Combine(WritePath, GetDefaultName());
    }

    // The user may not have provided a suitable extension
    if (FPathViews::GetExtension(WritePath) != TEXT("utrace"))
    {
        WritePath = FPaths::SetExtension(WritePath, TEXT(".utrace"));
    }

    // Finally make sure the path is platform friendly
    IFileManager& FileManager = IFileManager::Get();
    FString NativePath = FileManager.ConvertToAbsolutePathForExternalAppForWrite(*WritePath);

    // Ensure we can write the trace file appropriately
    const FString WriteDir = FPaths::GetPath(NativePath);
    if (!FPaths::IsDrive(WriteDir))
    {
        if (!FileManager.MakeDirectory(*WriteDir, true))
        {
            UE_LOG_REF(LogCategory, Warning, TEXT("Failed to create directory '%s'"), *WriteDir);
            return false;
        }
    }

    if (!bTruncateFile && FileManager.FileExists(*NativePath))
    {
        UE_LOG_REF(LogCategory, Warning, TEXT("Trace file '%s' already exists"), *NativePath);
        return false;
    }

    // Finally, tell trace to write the trace to a file.
    if (!UE::Trace::WriteTo(*NativePath))
    {
        if (FPathViews::Equals(NativePath, WritePath))
        {
            UE_LOG_REF(LogCategory, Warning, TEXT("Unable to trace to file '%s'"), *NativePath);
        }
        else
        {
            UE_LOG_REF(LogCategory, Warning, TEXT("Unable to trace to file '%s' (transformed from '%s')"), *NativePath, *WritePath);
        }
        return false;
    }

    TraceDest = MoveTemp(NativePath);
#endif
    return true;
}

////////////////////////////////////////////////////////////////////////////////
const char* FTraceAuxiliaryImpl::GetDest() const
{
    return TraceDest.c_str();
}

////////////////////////////////////////////////////////////////////////////////
bool FTraceAuxiliaryImpl::IsConnected() const
{
    return State == EState::Tracing;
}

bool FTraceAuxiliary::Start(EConnectionType type, const char* target, const char* channels, Options* options)
{
#if UE_TRACE_ENABLED
    if (GTraceAuxiliary.IsConnected())
    {
        //UE_LOG_REF(LogCategory, Error, TEXT("Unable to start trace, already tracing to %s"), GTraceAuxiliary.GetDest());
        return false;
    }

    // Make sure the worker thread is started unless explicitly opt out.
    if (!options || !options->bNoWorkerThread)
    {
        GTraceAuxiliary.StartWorkerThread();
    }

    if (channels)
    {
        //UE_LOG_REF(LogCategory, Display, TEXT("Requested channels: '%s'"), Channels);
        GTraceAuxiliary.ResetCommandlineChannels();
        GTraceAuxiliary.AddCommandlineChannels(channels);
        GTraceAuxiliary.EnableCommandlineChannels();

#if PLATFORM_ANDROID
        if (UE_TRACE_CHANNELEXPR_IS_ENABLED(GpuChannel))
        {
            GTraceAuxiliary.StartGpuTrace();
        }

        GTraceAuxiliary.InjectSysMemAlloc();
#endif
    }

    if (options)
    {
        // Truncation is only valid when tracing to file and filename is set.
        if (options->bTruncateFile && type == EConnectionType::File && target != nullptr)
        {
            GTraceAuxiliary.SetTruncateFile(options->bTruncateFile);
        }
    }

    if (type == EConnectionType::File)
    {
        return GTraceAuxiliary.Connect(ETraceConnectType::File, target);
    }
    else if (type == EConnectionType::Network)
    {
        return GTraceAuxiliary.Connect(ETraceConnectType::Network, target);
    }
#endif
    return false;
}

bool FTraceAuxiliary::Stop()
{
#if UE_TRACE_ENABLED
    return GTraceAuxiliary.Stop();
#else
    return false;
#endif
}

bool FTraceAuxiliary::Pause()
{
#if UE_TRACE_ENABLED
    GTraceAuxiliary.PauseChannels();
#endif
    return true;
}

bool FTraceAuxiliary::Resume()
{
#if UE_TRACE_ENABLED
    GTraceAuxiliary.ResumeChannels();
#endif
    return true;
}

////////////////////////////////////////////////////////////////////////////////
static bool StartFromCommandlineArguments()
{
#if UE_TRACE_ENABLED
    // Get active channels
    core::string Channels;
    if (ParseARGV("trace", Channels))
    {
    }
    else if (HasARGV("trace"))
    {
        Channels = GDefaultChannels;
    }

    core::string ShotInterval;
    if (ParseARGV("shot", ShotInterval))
    {
        auto ParsedInterval = StringToInt(ShotInterval);
        if (ParsedInterval > 50)
        {
            GScreenshotInterval = ParsedInterval;
        }
        else
        {
            WarningStringMsgWithoutStacktrace("'shot' argument should be greater than 50, current is %d.", ParsedInterval);
        }
    }

    // By default, if any channels are enabled we trace to memory.
    FTraceAuxiliary::EConnectionType Type = FTraceAuxiliary::EConnectionType::None;

    // Setup options
    FTraceAuxiliary::Options Opts;
    Opts.bTruncateFile = HasARGV("tracefiletrunc");
    Opts.bNoWorkerThread = false;

    // Find if a connection type is specified
    core::string Parameter;
    const char* Target = nullptr;
    if (ParseARGV("tracehost", Parameter))
    {
        Type = FTraceAuxiliary::EConnectionType::Network;
        Target = *Parameter;
    }
    else if (ParseARGV("tracefile", Parameter))
    {
        Type = FTraceAuxiliary::EConnectionType::File;
        if (Parameter.empty())
        {
            WarningStringMsgWithoutStacktrace("Empty parameter to 'tracefile' argument. Using default filename.");
            Target = nullptr;
        }
        else
        {
            Target = *Parameter;
        }
    }
    else if (HasARGV("tracefile"))
    {
        Type = FTraceAuxiliary::EConnectionType::File;
        Target = nullptr;
    }

    // If user has defined a connection type but not specified channels, use the default channel set.
    if (Type != FTraceAuxiliary::EConnectionType::None && Channels.empty())
    {
        Channels = GDefaultChannels;
    }

    if (Channels.empty())
    {
        return false;
    }
    LogStringMsg("Start Unreal Trace..");
    // Finally, start tracing to the requested connection.
    return FTraceAuxiliary::Start(Type, Target, *Channels, &Opts);
#endif
    return false;
}

void FTraceAuxiliary::Shutdown()
{
}

void FTraceAuxiliary::TryAutoConnect()
{
#if 0
#if UE_TRACE_ENABLED
    // Do not attempt to autoconnect when forking is requested.
    const bool bShouldAutoConnect = !FForkProcessHelper::IsForkRequested();
    if (bShouldAutoConnect)
    {
#if PLATFORM_WINDOWS
        // If we can detect a named event it means UnrealInsights (Browser Mode) is running.
        // In this case, we try to auto-connect with the Trace Server.
        HANDLE KnownEvent = ::OpenEvent(EVENT_ALL_ACCESS, false, TEXT("Local\\UnrealInsightsBrowser"));
        if (KnownEvent != nullptr)
        {
            UE_LOG(LogCore, Display, TEXT("Unreal Insights instance detected, auto-connecting to local trace server..."));
            Start(EConnectionType::Network, TEXT("127.0.0.1"), GTraceAuxiliary.HasCommandlineChannels() ? nullptr : TEXT("default"), nullptr);
            ::CloseHandle(KnownEvent);
        }
#endif
    }
#endif
#endif
}

void FTraceAuxiliary::EnableChannels()
{
}

void FTlsAutoCleanup::Register()
{
    static thread_local std::vector<std::unique_ptr<FTlsAutoCleanup>> TlsInstances;
    std::unique_ptr <FTlsAutoCleanup> instance;
    instance.reset(this);
    TlsInstances.push_back(std::move(instance));
}

void FTraceAuxiliaryImpl::StartGpuTrace()
{
#if PLATFORM_ANDROID
#if UE_TRACE_ENABLED
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(PerfettoChannel))
    {
        usleep(500000);
        LogStringWithoutStacktrace("Start to launch perfetto client...");
        GpuDataThread.SetName("GpuDataProducer");
        bPerfettoStopped.store(true);
        GpuDataThread.Run([](void* data) -> void*
        {
            FTraceAuxiliaryImpl* This = (FTraceAuxiliaryImpl*)data;
            usleep(500000);
            auto sockfd = UE::Trace::Private::TcpSocketConnect("127.0.0.1", 10018);
            if (!sockfd)
            {
                ErrorStringWithoutStacktrace("failed to connect GPUData socket.");
                goto end;
            }
            LogStringWithoutStacktrace("Start receving GPU data...");
            This->bPerfettoStopped.store(false);
            while (!This->bPerfettoStopped.load())
            {
                 if (UE::Trace::Private::TcpSocketHasData(sockfd))
                 {
                    //UTRACE_AUTO("Perfetto.RecvData");
                    uint32_t length = 0;
                    if (!UE::Trace::Private::IoRead(sockfd,  &length, sizeof(length)))
                    {
                        ErrorStringWithoutStacktrace("GPUDataProducer: failed to recv data length.");
                        break;
                    }
                    if (length > 0)
                    {   
                        //LogStringMsg("Try to recv %d bytes...", length);
                        dynamic_array<uint8_t> buffer(length);
                        if (!UE::Trace::Private::IoRead(sockfd, buffer.data(), (size_t)length))
                        {
                            ErrorStringWithoutStacktrace("GPUDataProducer: failed to recv data.");
                            break;
                        }

		                UE_TRACE_LOG(Perfetto, TracePacketChunk, PerfettoChannel)
			                << TracePacketChunk.Size(length)
			                << TracePacketChunk.Data(buffer.data(), length);

                    }
                 } // Has data
                 else
                 {
                    usleep(1000000);
                 }
            }
        end:
            UE::Trace::Private::IoClose(sockfd);
            return nullptr;
        }, this, 0);
    }
    else
    {
        WarningStringWithoutStacktrace("Perfetto disable while gpu trace enabled.");
    }
#endif // UE_TRACE_ENABLED
#endif
}

void FTraceAuxiliaryImpl::InjectSysMemAlloc()
{
#if PLATFORM_ANDROID
#if UE_TRACE_ENABLED
    //if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel))
    {
        if (HasARGV("memhook"))
        {
            utrace_module_init(MAKE_UTRACE_STR("so"), 0);

            auto libdl = dlopen("libdl.so", RTLD_GLOBAL);
            Fn_dl_register_notification dl_register_notification = (Fn_dl_register_notification)dlsym(libdl, "dl_register_notification");
            if (dl_register_notification)
            {
                dl_register_notification(&FTraceAuxiliaryImpl::OnDlNotify, this);
            }
            dl_iterate_phdr(&FTraceAuxiliaryImpl::OnDlIterate, this);

            //xhook_refresh(0);
        }
    }
#endif
#endif
}

#if PLATFORM_ANDROID
#if UE_TRACE_ENABLED
void FTraceAuxiliaryImpl::OnDlNotify(int flag, const char* so_name, const char* realpath, ElfW(Addr) base, const ElfW(Phdr)* phdr, ElfW(Half) phnum, void* data)
{
    for (int h = 0; h < phnum; h++)
        if ((phdr[h].p_type == PT_LOAD) &&
            (phdr[h].p_memsz > 0) &&
            (phdr[h].p_flags)) {
            const uintptr_t  first = base + phdr[h].p_vaddr;
            const uintptr_t  last  = first + phdr[h].p_memsz - 1;
            if (flag == 1)
            {
                FMemScope scope(MAKE_UTRACE_STR("Program"));
                utrace_module_load({ so_name, (utrace_u32)strlen(so_name) }, (utrace_u64)first, (utrace_u32)phdr[h].p_memsz, (UInt8*)realpath, (utrace_u32)strlen(realpath));
                /*auto callstack = */utrace_mem_alloc((utrace_u64)first, (utrace_u32)phdr[h].p_memsz, 0, programHeapId);
                //utrace_mem_mark_alloc_as_heap_with_callstack((utrace_u64)first, programHeapId, 0, callstack);
                //utrace_mem_alloc_with_callstack((utrace_u64)first, (utrace_u32)phdr[h].p_memsz, 0, 0, callstack);
            }
            else
            {
                utrace_module_unload((utrace_u64)first);
                utrace_mem_free((utrace_u64)first, 0);
            }
        }
}

struct NewSoInfo : public dl_phdr_info
{
    const char* realpath;
};

int FTraceAuxiliaryImpl::OnDlIterate(struct dl_phdr_info* info, size_t size, void* data)
{
    const char  *name;
    size_t      headers, h;
    const char  *realpath = nullptr;
    if (size == sizeof(NewSoInfo))
    {
        NewSoInfo* ninfo = (NewSoInfo*)info;
        realpath = ninfo->realpath;
    }
    /* Empty name refers to the binary itself. */
    if (!info->dlpi_name || !info->dlpi_name[0])
        name = (const char *)data;
    else
        name = info->dlpi_name;

    headers = info->dlpi_phnum;
    for (h = 0; h < headers; h++)
        if ((info->dlpi_phdr[h].p_type == PT_LOAD) &&
            (info->dlpi_phdr[h].p_memsz > 0) &&
            (info->dlpi_phdr[h].p_flags)) {
            FMemScope scope(MAKE_UTRACE_STR("Program"));
            const uintptr_t  first = info->dlpi_addr + info->dlpi_phdr[h].p_vaddr;
            utrace_module_load({ name, (utrace_u32)strlen(name) }, (utrace_u64)first, (utrace_u32)info->dlpi_phdr[h].p_memsz , (UInt8*)(realpath? realpath: NULL), realpath? (unsigned int)strlen(realpath): 0);
            /*auto callstack = */utrace_mem_alloc((utrace_u64)first, (utrace_u32)info->dlpi_phdr[h].p_memsz, 0, programHeapId);
            //utrace_mem_mark_alloc_as_heap_with_callstack((utrace_u64)first, programHeapId, 0, callstack);
            //utrace_mem_alloc_with_callstack((utrace_u64)first, (utrace_u32)info->dlpi_phdr[h].p_memsz, 0, 0, callstack);
        }

    return 0;
}
#endif
#endif

/////////////////////////// C API Interface ///////////////////////////////
void utrace_initialize_main_thread(
		utrace_string appName,
		utrace_string buildVersion,
		utrace_string commandLine,
		utrace_string branchName,
		unsigned int changeList,
		utrace_target_type target_type,
		utrace_build_configuration build_configuration,
        unsigned int tailBytes
) {
    if (GIsInitialized.load())
        return;

    core::ParseCommandline(commandLine.str, commandLine.len);

    core::string socName;
    core::string brandName;
    core::string osVer;
    core::string modelName;
    
#if PLATFORM_WINRT
    core::string platform = "WinRT";
#elif PLATFORM_XBOXONE
    core::string platform = "XboxOne";
#elif PLATFORM_WIN
    core::string platform = "Windows";
#elif PLATFORM_BJM
    core::string platform = "BJM";
#elif PLATFORM_LINUX
    core::string platform = "Linux";
#elif PLATFORM_OSX
    core::string platform = "MacOS";
#elif PLATFORM_IOS
    core::string platform = "iOS";
#elif PLATFORM_TVOS
    core::string platform = "tvOS";
#elif PLATFORM_WEBGL
    core::string platform = "WebGL";
#elif PLATFORM_ANDROID
    core::string platform = "Android";
#elif PLATFORM_PS4
    core::string platform = "PS4";
#elif PLATFORM_PS5
    core::string platform = "PS5";
#elif PLATFORM_SWITCH
    core::string platform = "Switch";
#elif PLATFORM_LUMIN
    core::string platform = "Lumin";
#else
    core::string platform = "Unknown";
#endif

    // TODO: core::string gpuName;
#if PLATFORM_ANDROID
    brandName = GetAndroidProp("ro.product.brand");
    osVer = GetAndroidProp("ro.build.version.release");
    socName = GetAndroidProp("ro.board.platform");
    modelName = GetAndroidProp("ro.product.model");
    //TODO: gpuName = GetAndroidProp("");
#endif
    UInt32 DataSize =
        (UInt32)platform.length() +
        appName.len +
        commandLine.len +
        branchName.len +
#if PLATFORM_ANDROID
        brandName.length() +
        osVer.length() +
        socName.length() +
        (UInt32)modelName.length() +
#endif
        buildVersion.len;
    UE_TRACE_LOG(Diagnostics, Session2, UE::Trace::TraceLogChannel, DataSize)
        << Session2.Platform(*platform, platform.length())
        << Session2.AppName(appName.str, appName.len)
        << Session2.CommandLine(commandLine.str, commandLine.len)
        << Session2.Branch(branchName.str, branchName.len)
        << Session2.BuildVersion(buildVersion.str, buildVersion.len)
#if PLATFORM_ANDROID
        << Session2.Brand(*brandName, brandName.length())
        << Session2.OSVer(*osVer, osVer.length())
        << Session2.SOC(*socName, socName.length())
        << Session2.Model(*modelName, modelName.length())
#endif
        << Session2.Changelist(changeList)
        << Session2.ConfigurationType(build_configuration)
        << Session2.TargetType(target_type);

    UE::Trace::FInitializeDesc Desc;
    Desc.bUseWorkerThread = false;

    if (tailBytes > 0)
    {
        Desc.TailSizeBytes = tailBytes;
        Desc.TailSizeBytes <<= 20;
    }

    UE::Trace::Initialize(Desc);
    GTraceAuxiliary.StartEndFramePump();
    GTraceAuxiliary.StartWorkerThread();

    UE::Trace::ThreadRegister("GameThread", Thread::mainThreadId, -1);
    StartFromCommandlineArguments();
    GIsInitialized.store(true);
}

void utrace_try_connect()
{
}

utrace_u64 utrace_get_timestamp()
{
    return UE::Trace::Private::TimeGetTimestamp();
}

int utrace_screenshot_enabled()
{
    return FMiscTrace::ShouldTraceScreenshot() ? 1: 0;
}

void utrace_register_thread(const char* name, utrace_u32 sysId, int sortHint)
{
    UE::Trace::ThreadRegister(name, sysId, sortHint);
}

void utrace_screenshot_write(
    utrace_string shotName,
    utrace_u64 timestamp, 
    utrace_u32 width, utrace_u32 height, 
    unsigned char* imagePNGData, utrace_u32 imagePNGSize
) {
    FMiscTrace::OutputScreenshot(
        core::string_ref(shotName.str, shotName.len), 
        timestamp, 
        width, height, 
        imagePNGData, imagePNGSize
    );
}

void utrace_cpu_begin_name(const char* name, const char* file, int line)
{
    FCpuProfilerTrace::OutputBeginDynamicEvent(name, /* pretty_funtion, */ file, line);
}

void utrace_cpu_begin_name2(const char* name, int name_len, const char* file_name, int file_name_len, int line)
{
    if (name_len == -1) {
        name_len = (int)strlen(name);
    }
    FCpuProfilerTrace::OutputBeginDynamicEvent2(name, name_len, file_name, file_name_len, line);
}

void utrace_cpu_end()
{
    FCpuProfilerTrace::OutputEndEvent();
}

void utrace_emit_render_frame(int is_begin)
{
    if (is_begin)
    {
        FMiscTrace::OutputBeginFrame(TraceFrameType_Rendering);
    }
    else
    {
        FMiscTrace::OutputEndFrame(TraceFrameType_Rendering);
    }
}

void utrace_emit_game_frame(int is_begin)
{
    if (is_begin)
    {
        FMiscTrace::OutputBeginFrame(TraceFrameType_Game);
    }
    else
    {
        FMiscTrace::OutputEndFrame(TraceFrameType_Game);
    }
}

int utrace_log_enabled()
{
    return UE_TRACE_CHANNELEXPR_IS_ENABLED(LogChannel) ? 1: 0;
}

void utrace_log_decl_category(const void* category, utrace_string name, utrace_log_type type)
{
    UE_TRACE_LOG(Logging, LogCategory, LogChannel, name.len)
        << LogCategory.CategoryPointer(category)
        << LogCategory.DefaultVerbosity(type)
        << LogCategory.Name(name.str, name.len);
}

void utrace_log_decl_spec(const void* logPoint, const void* category, utrace_log_type verbosity, utrace_string file, int line, const char* format)
{
    UInt16 FormatStringLen = UInt16(strlen(format));
    UInt32 DataSize = file.len + (FormatStringLen * sizeof(char));
    UE_TRACE_LOG(Logging, LogMessageSpec, LogChannel, DataSize)
        << LogMessageSpec.LogPoint(logPoint)
        << LogMessageSpec.CategoryPointer(category)
        << LogMessageSpec.Line(line)
        << LogMessageSpec.Verbosity(verbosity)
        << LogMessageSpec.FileName(file.str, file.len)
        << LogMessageSpec.FormatString(format, FormatStringLen);
}

void utrace_log_write_buffer(const void* logPoint, utrace_u16 encoded_arg_size, utrace_u8* encoded_arg_buffer)
{
    UE_TRACE_LOG(Logging, LogMessage, LogChannel)
        << LogMessage.LogPoint(logPoint)
        << LogMessage.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << LogMessage.FormatArgs(encoded_arg_buffer, encoded_arg_size);
}

void utrace_file_begin_open(utrace_string path)
{
    FPlatformFileTrace::BeginOpen(core::string_ref(path.str, path.len));
}

void utrace_file_fail_open(utrace_string path)
{
    FPlatformFileTrace::FailOpen(core::string_ref(path.str, path.len));
}

void utrace_file_begin_reopen(utrace_u64 oldHandle)
{
    FPlatformFileTrace::BeginReOpen(oldHandle);
}

void utrace_file_end_reopen(utrace_u64 newHandle)
{
    FPlatformFileTrace::EndReOpen(newHandle);
}

void utrace_file_end_open(utrace_u64 handle)
{
    FPlatformFileTrace::EndOpen(handle);
}

void utrace_file_begin_close(utrace_u64 handle)
{
    FPlatformFileTrace::BeginClose(handle);
}

void utrace_file_end_close(utrace_u64 handle)
{
    FPlatformFileTrace::EndClose(handle);
}

void utrace_file_fail_close(utrace_u64 handle)
{
    FPlatformFileTrace::FailClose(handle);
}

void utrace_file_begin_read(utrace_u64 readHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size)
{
    FPlatformFileTrace::BeginRead(readHandle, fileHandle, offset, size);
}

void utrace_file_end_read(utrace_u64 readHandle, utrace_u64 sizeRead)
{
    FPlatformFileTrace::EndRead(readHandle, sizeRead);
}

void utrace_file_begin_write(utrace_u64 writeHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size)
{
    FPlatformFileTrace::BeginWrite(writeHandle, fileHandle, offset, size);
}

void utrace_file_end_write(utrace_u64 writeHandle, utrace_u64 sizeWritten)
{
    FPlatformFileTrace::EndWrite(writeHandle, sizeWritten);
}

utrace_u16 utrace_counter_init(utrace_string name, utrace_counter_type type, utrace_counter_display_hint hint)
{
	if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(CountersChannel))
	{
		return 0;
	}
	static std::atomic<UInt16> NextId;
	UInt16 CounterId = UInt16(NextId++) + 1;
	UInt16 NameLen = name.len;
	UE_TRACE_LOG(Counters, Spec, CountersChannel, NameLen)
		<< Spec.Id(CounterId)
		<< Spec.Type(UInt8(type))
		<< Spec.DisplayHint(UInt8(hint))
		<< Spec.Name(name.str, NameLen);
	return CounterId;
}

void utrace_counter_set_int(utrace_u16 counter_id, utrace_i64 value)
{
	if (!counter_id)
	{
		return;
	}

	UE_TRACE_LOG(Counters, SetValueInt, CountersChannel)
		<< SetValueInt.Cycle(UE::Trace::Private::TimeGetTimestamp())
		<< SetValueInt.Value(value)
		<< SetValueInt.CounterId(counter_id);
}

void utrace_counter_set_float(utrace_u16 counter_id, double value)
{
	if (!counter_id)
	{
		return;
	}
	UE_TRACE_LOG(Counters, SetValueFloat, CountersChannel)
		<< SetValueFloat.Cycle(UE::Trace::Private::TimeGetTimestamp())
		<< SetValueFloat.Value(value)
		<< SetValueFloat.CounterId(counter_id);
}

void utrace_module_init(utrace_string fmt, unsigned char shift)
{
    if (GIsModuleInitialized)
        return;
	UE_TRACE_LOG(Diagnostics, ModuleInit, ModuleChannel, sizeof(char) * 3)
		<< ModuleInit.SymbolFormat(fmt.str, fmt.len)
		<< ModuleInit.ModuleBaseShift(shift);
    GIsModuleInitialized.store(true);
}

void utrace_module_load(utrace_string name, utrace_u64 base, utrace_u32 size, const utrace_u8* imageIdData, utrace_u32 imageIdLen)
{
	UE_TRACE_LOG(Diagnostics, ModuleLoad, ModuleChannel, name.len + imageIdLen)
		<< ModuleLoad.Name(name.str, name.len)
		<< ModuleLoad.Base(base)
		<< ModuleLoad.Size(size)
		<< ModuleLoad.ImageId(imageIdData, imageIdLen);
}

void utrace_module_unload(utrace_u64 base)
{
	UE_TRACE_LOG(Diagnostics, ModuleUnload, ModuleChannel)
		<< ModuleUnload.Base(base);
}

utrace_u32 utrace_current_callstack_id()
{
    return GetCallstackId();
}

void utrace_mem_trace_virtual_alloc(int traced)
{
#if PLATFORM_ANDROID
    int *Need = (int *)pthread_getspecific(GNeedHookMalloc);
    if (!Need)
    {
        Need = (int*)malloc(sizeof(int) * 2);
        pthread_setspecific(GNeedHookMalloc, Need);
    }
    *(Need + 1) = traced;
#endif
}

///////////////////////////////////////////////////////////////////////////

#include "Runtime/TraceLog/Private/Trace/BlockPool.cpp"
#include "Runtime/TraceLog/Private/Trace/Channel.cpp"
#include "Runtime/TraceLog/Private/Trace/Codec.cpp"
#include "Runtime/TraceLog/Private/Trace/Control.cpp"
#include "Runtime/TraceLog/Private/Trace/EventNode.cpp"
#include "Runtime/TraceLog/Private/Trace/Field.cpp"
#include "Runtime/TraceLog/Private/Trace/Tail.cpp"
#include "Runtime/TraceLog/Private/Trace/TlsBuffer.cpp"
#include "Runtime/TraceLog/Private/Trace/Trace.cpp"
#include "Runtime/TraceLog/Private/Trace/Writer.cpp"
#include "Runtime/TraceLog/Private/Trace/Important/Cache.cpp"
#include "Runtime/TraceLog/Private/Trace/Important/SharedBuffer.cpp"

#if PLATFORM_WIN
#include "Runtime/TraceLog/Private/Trace/Detail/Windows/WindowsTrace.cpp"
#elif PLATFORM_LINUX
#elif PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_OSX
#include "Runtime/TraceLog/Private/Trace/Detail/Apple/AppleTrace.cpp"
#elif PLATFORM_ANDROID
#if !BUILD_UTRACE_LIB
    #include "Runtime/TraceLog/Private/Trace/Detail/Android/AndroidTrace.cpp"
#endif

extern "C" {
extern void *(*origin_mmap)(void *addr, size_t length, int prot, int flags,
                     int fd, off_t offset);
extern int (*origin_munmap)(void *addr, size_t length);

typedef void (*Fn_mmap_hook)          (void*, void* addr, size_t size, int prot, int flags, int fd, off_t offset, void* data);
typedef void (*Fn_munmap_hook)        (int, void* addr, size_t size, void* data);

typedef void (*Fn_calloc_prehook)        (size_t n_elements, size_t elem_size, void* data);
typedef void (*Fn_malloc_prehook)        (size_t bytes, void* data);
typedef void (*Fn_free_prehook)          (void* mem, void* data);
typedef void (*Fn_memalign_prehook)      (size_t alignment, size_t bytes, void* data);
typedef void (*Fn_posix_memalign_prehook)(void** memptr, size_t alignment, size_t size, void* data);
typedef void (*Fn_aligned_alloc_prehook) (size_t alignment, size_t size, void* data);
typedef void (*Fn_realloc_prehook)       (void* old_mem, size_t bytes, void* data);

typedef void (*Fn_calloc_hook)        (void*, size_t n_elements, size_t elem_size, void* data);
typedef void (*Fn_malloc_hook)        (void*, size_t bytes, void* data);
typedef void (*Fn_free_hook)          (void* ptr, void* data);
typedef void (*Fn_memalign_hook)      (void*, size_t alignment, size_t bytes, void* data);
typedef void (*Fn_posix_memalign_hook)(int,   void** memptr, size_t alignment, size_t size, void* data);
typedef void (*Fn_aligned_alloc_hook) (void*, size_t alignment, size_t size, void* data);
typedef void (*Fn_realloc_hook)       (void*, void* old_mem, size_t bytes, void* data);

struct libc_memhooks {
    void*                     usr_data = nullptr;

    Fn_mmap_hook              mmap_hook = nullptr;
    Fn_munmap_hook            munmap_hook = nullptr;

    Fn_free_prehook           free_prehook = nullptr;
    Fn_free_hook              free_hook = nullptr;

    Fn_calloc_prehook         calloc_prehook = nullptr;
    Fn_calloc_hook            calloc_hook = nullptr;

    Fn_malloc_prehook         malloc_prehook = nullptr;
    Fn_malloc_hook            malloc_hook = nullptr;

    Fn_memalign_prehook       memalign_prehook = nullptr;
    Fn_memalign_hook          memalign_hook = nullptr;

    Fn_aligned_alloc_prehook  aligned_alloc_prehook = nullptr;
    Fn_aligned_alloc_hook     aligned_alloc_hook = nullptr;

    Fn_posix_memalign_prehook posix_memalign_prehook = nullptr;
    Fn_posix_memalign_hook    posix_memalign_hook = nullptr;

    Fn_realloc_prehook        realloc_prehook = nullptr;
    Fn_realloc_hook           realloc_hook = nullptr;
};

typedef void (*Fn_register_memory_hooks)(libc_memhooks* hooks);
};

static void mmap_hook   (void*, void* addr, size_t size, int prot, int flags, int fd, off_t offset, void* data);
static void munmap_hook (int, void* addr, size_t size, void* data);

static void free_prehook  (void* mem, void* data);
static void free_hook     (void* mem, void* data);
static void calloc_prehook(size_t n_elements, size_t elem_size, void* data);
static void calloc_hook   (void* ptr, size_t n_elements, size_t elem_size, void* data);
static void malloc_prehook(size_t bytes, void* data);
static void malloc_hook   (void* ptr, size_t bytes, void* data);
static void memalign_prehook(size_t alignment, size_t bytes, void* data);
static void memalign_hook(void*,size_t alignment, size_t bytes, void* data);
//static void aligned_alloc_prehook(size_t alignment, size_t size, void* data);
//static void aligned_alloc_hook(void*, size_t alignment, size_t size, void* data);
static void posix_memalign_prehook(void** memptr, size_t alignment, size_t size, void* data);
static void posix_memalign_hook(int,void** memptr, size_t alignment, size_t size, void* data);
static void realloc_prehook(void* old_mem, size_t bytes, void* data);
static void realloc_hook(void*, void* old_mem, size_t bytes, void* data);

static thread_local bool shouldTraceVirtual = true;

__attribute__ ((constructor (101))) void construct0()
{
    utrace_mem_init_tags();
    char prop[128] = {0};
    pthread_key_create(&GNeedHookMalloc, [] (void* key) {});
    if (__system_property_get("debug.utrace.callstack.fastcapture", prop) > 0)
    {
        GUseFastCallStackTrace = !strcmp(prop, "true") || !strcmp(prop, "1");
    }
    if (__system_property_get("debug.utrace.callstack.enable", prop) > 0)
    {
        GCaptureCallStack = !strcmp(prop, "true") || !strcmp(prop, "1");
    }

    auto libc = dlopen("libc.so", RTLD_LAZY);
    if (libc)
    {
        auto pmmap = dlsym(libc, "__private_mmap");
        if (pmmap)
        {
            origin_mmap = decltype(origin_mmap)(pmmap);
        }
        auto pmunmap = dlsym(libc, "__private_munmap");
        if (pmunmap)
        {
            origin_munmap = decltype(origin_munmap)(pmunmap);
        }
        auto register_memhooks = (Fn_register_memory_hooks)dlsym(libc, "__register_memory_hooks");
        if (register_memhooks)
        {
            libc_memhooks hooks = {
                nullptr,
                mmap_hook,
                munmap_hook,
                free_prehook,
                free_hook,
                calloc_prehook,
                calloc_hook,
                malloc_prehook,
                malloc_hook,
                memalign_prehook,
                memalign_hook,
                memalign_prehook,
                memalign_hook,
                posix_memalign_prehook,
                posix_memalign_hook,
                realloc_prehook,
                realloc_hook,
            };
            register_memhooks(&hooks);
        }
    }

}

void DisableMallocHook(bool disable)
{
    int *Need = (int *)pthread_getspecific(GNeedHookMalloc);
    if (!Need)
    {
        Need = (int*)malloc(sizeof(int) * 2);
        pthread_setspecific(GNeedHookMalloc, Need);
    }
    *Need = disable ? 1: 0;
}

bool IsMallocHookDisabled()
{
    int *Need = (int *)pthread_getspecific(GNeedHookMalloc);
    if (!Need)
    {
        Need = (int*)malloc(sizeof(int) * 2);
        *Need = 0;
        *(Need + 1) = 0;
        pthread_setspecific(GNeedHookMalloc, Need);
    }
    return *Need == 1;
}

FORCEINLINE bool IsMmapHookDisabled()
{
    int *Need = (int *)pthread_getspecific(GNeedHookMalloc);
    if (!Need)
    {
        Need = (int*)malloc(sizeof(int) * 2);
        *Need = 0;
        *(Need + 1) = 0;
        pthread_setspecific(GNeedHookMalloc, Need);
    }
    return *(Need+1) == 0;
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = 0;
    vm->AttachCurrentThread(&env, 0);
    char prop[1024] = {0};
    if (__system_property_get("debug.utrace.commandline", prop) > 0)
    {
        std::string commandline(prop);
        auto read_app_name = []() -> std::string {
            int fd = open("/proc/self/cmdline", O_RDONLY|O_SYNC);
            char buffer[256] = {0};
            size_t len = read(fd, buffer, 256);
            close(fd);
            return std::string(buffer, len);
        };
        auto app_name = read_app_name();
        // read app name "/proc/self/cmdline"
        utrace_initialize_main_thread(
        { app_name.c_str(), (utrace_u32)app_name.length() },
        MAKE_UTRACE_STR("unknown"),
        { commandline.c_str(), (utrace_u32)commandline.size() },
        MAKE_UTRACE_STR("unkown"),
        0,
        UTRACE_TARGET_TYPE_GAME,
        UTRACE_BUILD_CONFIGURATION_DEVELOPMENT,
        0
        );
    }
    utrace_mem_init();
    utrace_module_init(MAKE_UTRACE_STR("so"), 0);
    auto libdl = dlopen("libdl.so", RTLD_GLOBAL);
    Fn_dl_register_notification dl_register_notification = (Fn_dl_register_notification)dlsym(libdl, "dl_register_notification");
    if (dl_register_notification)
    {
        dl_register_notification(&FTraceAuxiliaryImpl::OnDlNotify, nullptr);
    }
    dl_iterate_phdr(&FTraceAuxiliaryImpl::OnDlIterate, nullptr);

    return JNI_VERSION_1_6;
}

void mmap_hook   (void* ptr, void* addr, size_t size, int prot, int flags, int fd, off_t offset, void* data)
{
    if (!GTraceAllowed || IsMmapHookDisabled())
    {
        return;
    }
    if (fd == -1 && offset == 0)
    {
        FMemScope scope(MAKE_UTRACE_STR("VirtualAlloc"));
        /*utrace_u32 callStackId = */utrace_mem_alloc(UInt64(ptr), size, 0, 0);
        //utrace_mem_mark_alloc_as_heap_with_callstack(UInt64(ptr), 0, 0, callStackId);
    }
}

void munmap_hook (int ret, void* addr, size_t size, void* data)
{
    if (!GTraceAllowed || IsMmapHookDisabled())
    {
        return;
    }
    utrace_u32 callStackId = utrace_mem_free((UInt64)addr, 0);
    //utrace_mem_unmark_alloc_as_heap_with_callstack((UInt64)addr, 0, callStackId);
}

void free_prehook(void* mem, void* data)
{
    utrace_mem_trace_virtual_alloc(0);
}

// TODO: should use sampling algorithm

void free_hook(void* mem, void* data)
{
    if (!IsMallocHookDisabled())
    {
        utrace_mem_free((utrace_u64)mem, libcHeapId);
    }
    utrace_mem_trace_virtual_alloc(1);
}

void calloc_prehook(size_t n_elements, size_t elem_size, void* data)
{
    utrace_mem_trace_virtual_alloc(0);
}

void calloc_hook(void* ptr, size_t n_elements, size_t elem_size, void* data)
{
    FMemScope scope(MAKE_UTRACE_STR("LibC"));
    utrace_mem_alloc((utrace_u64)ptr, n_elements*elem_size, 0, libcHeapId);
    utrace_mem_trace_virtual_alloc(1);
}

void malloc_prehook(size_t bytes, void*)
{
    utrace_mem_trace_virtual_alloc(0);
}

void malloc_hook(void* ptr, size_t bytes, void*)
{
    if (!IsMallocHookDisabled())
    {
        FMemScope scope(MAKE_UTRACE_STR("LibC"));
        utrace_mem_alloc((utrace_u64)ptr, bytes, 0, libcHeapId);
    }
    utrace_mem_trace_virtual_alloc(1);
}

void memalign_prehook(size_t alignment, size_t bytes, void* data)
{
    utrace_mem_trace_virtual_alloc(0);
}

void memalign_hook(void* ptr,size_t alignment, size_t bytes, void* data)
{
    FMemScope scope(MAKE_UTRACE_STR("LibC"));
    utrace_mem_alloc((utrace_u64)ptr, bytes, alignment, libcHeapId);
    utrace_mem_trace_virtual_alloc(1);
}

void posix_memalign_prehook(void** memptr, size_t alignment, size_t size, void* data)
{
    utrace_mem_trace_virtual_alloc(0);
}

void posix_memalign_hook(int, void** ptr, size_t alignment, size_t size, void* data)
{
    if (ptr)
    {
        FMemScope scope(MAKE_UTRACE_STR("LibC"));
        utrace_mem_alloc((utrace_u64)*ptr, size, alignment, libcHeapId);
    }
    utrace_mem_trace_virtual_alloc(1);
}

void realloc_prehook(void* old_mem, size_t bytes, void* data)
{
    utrace_mem_trace_virtual_alloc(0);
    utrace_mem_realloc_free((utrace_u64)old_mem, libcHeapId);
}

void realloc_hook(void* ptr, void* old_mem, size_t bytes, void* data)
{
    FMemScope scope(MAKE_UTRACE_STR("LibC"));
    utrace_mem_realloc_alloc((utrace_u64)ptr, bytes, 0, libcHeapId);
    utrace_mem_trace_virtual_alloc(1);
}
#else
#endif

#endif // LILITH_UNITY_UNREAL_INSIGHTS

//#pragma optimize("",on)
