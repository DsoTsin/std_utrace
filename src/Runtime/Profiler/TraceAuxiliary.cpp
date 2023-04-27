#include "Prefix.h"

#include "utrace.h"
#include "TraceAuxiliary.h"

#include "GrowOnlyLockFreeHash.h"

#include "UTrace/GPUTrace.h"
#include "UTrace/FileTrace.h"
#include "UTrace/CounterTrace.h"
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>

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

UE_TRACE_EVENT_BEGIN(Memory, Free)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, FreeSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, FreeVideo)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
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
    UE_TRACE_EVENT_FIELD(UInt8, RootHeap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, ReallocFreeSystem)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
UE_TRACE_EVENT_END()

//UE_TRACE_CHANNEL_DEFINE(CallstackChannel)
//
//UE_TRACE_EVENT_BEGIN(Memory, CallstackSpec, NoSync)
//	UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
//	UE_TRACE_EVENT_FIELD(UInt64[], Frames)
//UE_TRACE_EVENT_END()

#if 0
UE_TRACE_EVENT_BEGIN(Memory, HeapSpec, NoSync | Important)
    UE_TRACE_EVENT_FIELD(HeapId, Id)
    UE_TRACE_EVENT_FIELD(HeapId, ParentId)
    UE_TRACE_EVENT_FIELD(UInt16, Flags)
    UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, HeapMarkAlloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(UInt16, Flags)
    UE_TRACE_EVENT_FIELD(HeapId, Heap)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(Memory, HeapUnmarkAlloc)
    UE_TRACE_EVENT_FIELD(UInt64, Address)
    UE_TRACE_EVENT_FIELD(HeapId, Heap)
UE_TRACE_EVENT_END()
#endif

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

// If layout of the above events are changed, bump this version number
constexpr UInt8 MemoryTraceVersion = 1;

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

UInt16 FCountersTrace::OutputInitCounter(const char* CounterName, ETraceCounterType CounterType, ETraceCounterDisplayHint CounterDisplayHint)
{
    if (!UE_TRACE_CHANNELEXPR_IS_ENABLED(CountersChannel))
    {
        return 0;
    }

    static std::atomic<UInt16> NextId;

    UInt16 CounterId = UInt16(NextId++) + 1;
    UInt16 NameLen = UInt16(strlen(CounterName));
    UE_TRACE_LOG(Counters, Spec, CountersChannel, NameLen * sizeof(char))
        << Spec.Id(CounterId)
        << Spec.Type(UInt8(CounterType))
        << Spec.DisplayHint(UInt8(CounterDisplayHint))
        << Spec.Name(CounterName, NameLen);
    return CounterId;
}

void FCountersTrace::OutputSetValue(uint16_t CounterId, int64_t Value)
{
    if (!CounterId)
    {
        return;
    }

    UE_TRACE_LOG(Counters, SetValueInt, CountersChannel)
        << SetValueInt.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << SetValueInt.Value(Value)
        << SetValueInt.CounterId(CounterId);
}

void FCountersTrace::OutputSetValue(uint16_t CounterId, double Value)
{
    if (!CounterId)
    {
        return;
    }

    UE_TRACE_LOG(Counters, SetValueFloat, CountersChannel)
        << SetValueFloat.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << SetValueFloat.Value(Value)
        << SetValueFloat.CounterId(CounterId);
}

void FLogTrace::OutputLogCategory(const void* LogPoint, const char* Name, ELogVerbosity::Type DefaultVerbosity)
{
    UInt16 NameLen = UInt16(strlen(Name));
    UE_TRACE_LOG(Logging, LogCategory, LogChannel, NameLen * sizeof(char))
        << LogCategory.CategoryPointer(LogPoint)
        << LogCategory.DefaultVerbosity(DefaultVerbosity)
        << LogCategory.Name(Name, NameLen);
}

void FLogTrace::OutputLogMessageSpec(const void* LogPoint, const void* Category, ELogVerbosity::Type Verbosity, const char* File, int32_t Line, const char* Format)
{
    UInt16 FileNameLen = UInt16(strlen(File));
    UInt16 FormatStringLen = UInt16(strlen(Format));
    UInt32 DataSize = (FileNameLen * sizeof(char)) + (FormatStringLen * sizeof(char));
    UE_TRACE_LOG(Logging, LogMessageSpec, LogChannel, DataSize)
        << LogMessageSpec.LogPoint(LogPoint)
        << LogMessageSpec.CategoryPointer(Category)
        << LogMessageSpec.Line(Line)
        << LogMessageSpec.Verbosity(Verbosity)
        << LogMessageSpec.FileName(File, FileNameLen)
        << LogMessageSpec.FormatString(Format, FormatStringLen);
}

void FLogTrace::OutputLogMessageInternal(const void* LogPoint, uint16_t EncodedFormatArgsSize, uint8_t* EncodedFormatArgs)
{
    UE_TRACE_LOG(Logging, LogMessage, LogChannel)
        << LogMessage.LogPoint(LogPoint)
        << LogMessage.Cycle(UE::Trace::Private::TimeGetTimestamp())
        << LogMessage.FormatArgs(EncodedFormatArgs, EncodedFormatArgsSize);
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

void utrace_mem_update_internal();

void utrace_mem_init()
{
#if UE_TRACE_ENABLED
    if (!TRACE_PRIVATE_CHANNELEXPR_IS_ENABLED(MemAllocChannel))
    {
        return;
    }

    atexit([]() { GDoPumpTrace = true; });

    GTraceAllowed = true;

    UE_TRACE_LOG(Memory, Init, MemAllocChannel)
        << Init.MarkerPeriod(MarkerSamplePeriod + 1)
        << Init.Version(MemoryTraceVersion)
        << Init.MinAlignment(UInt8(MIN_ALIGNMENT))
        << Init.SizeShift(UInt8(SizeShift));
    //const HeapId SystemRootHeap = MemoryTrace_RootHeapSpec(TEXT("System memory"));
    //check(SystemRootHeap == EMemoryTraceRootHeap::SystemMemory);
    //const HeapId VideoRootHeap = MemoryTrace_RootHeapSpec(TEXT("Video memory"));
    //check(VideoRootHeap == EMemoryTraceRootHeap::VideoMemory);
    static_assert((1 << SizeShift) - 1 <= MIN_ALIGNMENT, "Not enough bits to pack size fields");
#endif
}

void utrace_mem_alloc(UInt64 Address, UInt64 Size, UInt32 Alignment)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }
    const UInt32 AlignmentPow2 = UInt32(CountTrailingZeros(Alignment));
    const UInt32 Alignment_SizeLower = (AlignmentPow2 << SizeShift) | UInt32(Size & ((1 << SizeShift) - 1));
    const UInt32 CallstackId = 0; // GDoNotAllocateInTrace ? 0 : CallstackTrace_GetCurrentId();
    UE_TRACE_LOG(Memory, AllocSystem, MemAllocChannel)
        << AllocSystem.CallstackId(CallstackId)
        << AllocSystem.Address(UInt64(Address))
        << AllocSystem.Size(UInt32(Size >> SizeShift))
        << AllocSystem.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower));

    utrace_mem_update_internal();
#endif
}

void utrace_mem_free(UInt64 Address)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }
    UE_TRACE_LOG(Memory, FreeSystem, MemAllocChannel)
        << FreeSystem.Address(UInt64(Address));

    utrace_mem_update_internal();
#endif
}

void utrace_mem_realloc_free(UInt64 Address)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }
    UE_TRACE_LOG(Memory, ReallocFreeSystem, MemAllocChannel)
        << ReallocFreeSystem.Address(UInt64(Address));

    utrace_mem_update_internal();
#endif
}

void utrace_mem_realloc_alloc(UInt64 Address, UInt64 NewSize, UInt32 Alignment)
{
#if UE_TRACE_ENABLED
    if (!GTraceAllowed)
    {
        return;
    }
    const UInt32 AlignmentPow2 = UInt32(CountTrailingZeros(Alignment));
    const UInt32 Alignment_SizeLower = (AlignmentPow2 << SizeShift) | UInt32(NewSize & ((1 << SizeShift) - 1));
    const UInt32 CallstackId = 0; //GDoNotAllocateInTrace ? 0 : CallstackTrace_GetCurrentId();
    UE_TRACE_LOG(Memory, ReallocAllocSystem, MemAllocChannel)
        << ReallocAllocSystem.CallstackId(CallstackId)
        << ReallocAllocSystem.Address(UInt64(Address))
        << ReallocAllocSystem.Size(UInt32(NewSize >> SizeShift))
        << ReallocAllocSystem.AlignmentPow2_SizeLower(UInt8(Alignment_SizeLower));

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

#if UE_TRACE_ENABLED
class FTagTrace
{
public:
#if 0
    FTagTrace(FMalloc* InMalloc);
#endif
    void			AnnounceGenericTags() const;
    void 			AnnounceTagDeclarations();
    void AnnounceSpecialTags() const;
#if ENABLE_LOW_LEVEL_MEM_TRACKER
    static void 	OnAnnounceTagDeclaration(FLLMTagDeclaration& TagDeclaration);
#endif
    SInt32			AnnounceCustomTag(SInt32 Tag, SInt32 ParentTag, const char* Display) const;
#if 0
    SInt32 			AnnounceFNameTag(const FName& TagName);
#endif
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
#if 0
    typedef TGrowOnlyLockFreeHash<FTagNameSetEntry, SInt32, bool> FTagNameSet;

    FTagNameSet				AnnouncedNames;
    static FMalloc* Malloc;
#endif
};

#if 0
FMalloc* FTagTrace::Malloc = nullptr;
static FTagTrace* GTagTrace = nullptr;
////////////////////////////////////////////////////////////////////////////////
FTagTrace::FTagTrace(FMalloc* InMalloc)
    : AnnouncedNames(InMalloc)
{
    Malloc = InMalloc;
    AnnouncedNames.Reserve(1024);
    AnnounceGenericTags();
    AnnounceTagDeclarations();
    AnnounceSpecialTags();
}
#endif

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
#if ENABLE_LOW_LEVEL_MEM_TRACKER
void FTagTrace::OnAnnounceTagDeclaration(FLLMTagDeclaration& TagDeclaration)
{
    TagDeclaration.ConstructUniqueName();
    GTagTrace->AnnounceFNameTag(TagDeclaration.GetUniqueName());
}
#endif

#if 0
////////////////////////////////////////////////////////////////////////////////
SInt32 FTagTrace::AnnounceFNameTag(const FName& Name)
{
    const SInt32 NameIndex = Name.GetDisplayIndex().ToUnstableInt() + FNAME_INDEX_OFFSET;

    // Find or add the item
    bool bAlreadyInTable;
    AnnouncedNames.FindOrAdd(NameIndex, true, &bAlreadyInTable);
    if (bAlreadyInTable)
    {
        return NameIndex;
    }

    // First time encountering this name, announce it
    ANSICHAR NameString[NAME_SIZE];
    Name.GetPlainANSIString(NameString);

    SInt32 ParentTag = -1;
    FAnsiStringView NameView(NameString);
    SInt32 LeafStart;
    if (NameView.FindLastChar('/', LeafStart))
    {
        FName ParentName(NameView.Left(LeafStart));
        ParentTag = AnnounceFNameTag(ParentName);
    }

    return AnnounceCustomTag(NameIndex, ParentTag, NameString);
}
#endif
////////////////////////////////////////////////////////////////////////////////
SInt32 FTagTrace::AnnounceCustomTag(SInt32 Tag, SInt32 ParentTag, const char* Display) const
{
    const UInt32 DisplayLen = 0;// FCStringAnsi::Strlen(Display);
    UE_TRACE_LOG(Memory, TagSpec, MemAllocChannel, DisplayLen)
        << TagSpec.Tag(Tag)
        << TagSpec.Parent(ParentTag)
        << TagSpec.Display(Display, DisplayLen);
    return Tag;
}


FMemScope::FMemScope(SInt32 InTag, bool bShouldActivate /*= true*/)
{
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate)
    {
        ActivateScope(InTag);
    }
}

#if 0
////////////////////////////////////////////////////////////////////////////////
FMemScope::FMemScope(ELLMTag InTag, bool bShouldActivate /*= true*/)
{
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate)
    {
        ActivateScope(static_cast<SInt32>(InTag));
    }
}

////////////////////////////////////////////////////////////////////////////////
FMemScope::FMemScope(const FName& InName, bool bShouldActivate /*= true*/)
{
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate)
    {
        ActivateScope(MemoryTrace_AnnounceFNameTag(InName));
    }
}

////////////////////////////////////////////////////////////////////////////////
FMemScope::FMemScope(const UE::LLMPrivate::FTagData* TagData, bool bShouldActivate /*= true*/)
{
#if ENABLE_LOW_LEVEL_MEM_TRACKER
    if (UE_TRACE_CHANNELEXPR_IS_ENABLED(MemAllocChannel) & bShouldActivate && TagData)
    {
        ActivateScope(MemoryTrace_AnnounceFNameTag(TagData->GetName()));
    }
#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
}
#endif

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

#endif // End UE_TRACE_ENABLED

SInt32 utrace_mem_announce_tag(SInt32 Tag, SInt32 ParentTag, const char* Display)
{
#if UE_TRACE_ENABLED
    return GActiveTag;
#else
    return -1;
#endif
}

SInt32 utrace_mem_name_tag(const char* TagName)
{
#if UE_TRACE_ENABLED
    return GActiveTag;
#else
    return -1;
#endif
}

SInt32 utrace_mem_get_active_tag()
{
#if UE_TRACE_ENABLED
    return GActiveTag;
#else
    return -1;
#endif
}

void utrace_screenshot(int frameNo)
{
#if UE_TRACE_ENABLED
    if (GScreenshotInterval > 0 && (frameNo % GScreenshotInterval == 0))
    {
#if 0
        FMiscTrace::Screenshot(frameNo, GetGfxDevice());
#endif
    }
#endif
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

const char* GDefaultChannels = "cpu,gpu,frame,log,file,bookmark"; // TODO: memalloc,

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
                if (Token.compare("default"/*, kComparisonIgnoreCase*/) == 0)
                {
                    ForEachChannel(GDefaultChannels, false, Callable);
                    return;
                }
                //else if (Token.compare("memory", kComparisonIgnoreCase) == 0)
                //{
                //    ForEachChannel(GMemoryChannels, false, Callable);
                //}
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

void utrace_register_thread(const char* name, unsigned int sysId, int sortHint)
{
    UE::Trace::ThreadRegister(name, sysId, sortHint);
}

void utrace_write_screenshot(
		utrace_string shotName,
		unsigned long long timestamp, 
		unsigned int width, unsigned int height, 
		unsigned char* imagePNGData, unsigned int imagePNGSize
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

void utrace_module_init(utrace_string fmt, unsigned char shift)
{
	UE_TRACE_LOG(Diagnostics, ModuleInit, ModuleChannel, sizeof(char) * 3)
		<< ModuleInit.SymbolFormat(fmt.str, fmt.len)
		<< ModuleInit.ModuleBaseShift(shift);
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
    using namespace UE::Trace;
    static thread_local void* buffer[CALLSTACK_BUFFERSIZE];
    UInt32 hash = 0;
    auto numStacks = UE::Trace::Private::GetCallbackFrames(buffer, CALLSTACK_BUFFERSIZE, 0, hash, true);
    void* AddressOfReturnAddress = PLATFORM_RETURN_ADDRESS_POINTER();
    if (FBacktracer* Instance = FBacktracer::Get())
    {
        return Instance->GetBacktraceId(hash, numStacks, buffer);
    }
    return 0;
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
#else
#endif

#endif // LILITH_UNITY_UNREAL_INSIGHTS

//#pragma optimize("",on)
