#pragma once

#if LILITH_UNITY_UNREAL_INSIGHTS
#include "Runtime/TraceLog/Public/Trace/Trace.h"
#include "Runtime/TraceLog/Public/Trace/Detail/Channel.h"
#include "Runtime/TraceLog/Public/Trace/Detail/Channel.inl"

enum ETraceFrameType
{
    TraceFrameType_Game,
    TraceFrameType_Rendering,

    TraceFrameType_Count
};

struct FTraceUtils
{
    static void Encode7bit(uint64_t Value, uint8_t*& BufferPtr)
    {
        do
        {
            uint8_t HasMoreBytes = (uint8_t)((Value > uint64_t(0x7F)) << 7);
            *(BufferPtr++) = (uint8_t)(Value & 0x7F) | HasMoreBytes;
            Value >>= 7;
        } while (Value > 0);
    }

    static void EncodeZigZag(int64_t Value, uint8_t*& BufferPtr)
    {
        Encode7bit((Value << 1) ^ (Value >> 63), BufferPtr);
    }
};

#if UE_TRACE_ENABLED
UE_TRACE_CHANNEL_EXTERN(CpuChannel);
UE_TRACE_CHANNEL_EXTERN(CountersChannel);
UE_TRACE_CHANNEL_EXTERN(LogChannel);

// Advanced macro for integrating e.g. stats/named events with cpu trace
// Declares a cpu timing event for future use, conditionally and with a particular variable name for storing the id
#define TRACE_CPUPROFILER_EVENT_DECLARE(NameStr, DeclName, Channel, Condition) \
	static uint32_t DeclName; \
	if ((Condition) && bool(Channel|CpuChannel) && (DeclName == 0)) { \
		DeclName = FCpuProfilerTrace::OutputEventType(NameStr, __FILE__, __LINE__); \
	}

// Advanced macro for integrating e.g. stats/named events with cpu trace
// Traces a scoped event previously declared with TRACE_CPUPROFILER_EVENT_DECLARE, conditionally
#define TRACE_CPUPROFILER_EVENT_SCOPE_USE(DeclName, ScopeName, Channel, Condition) \
	FCpuProfilerTrace::FEventScope ScopeName(DeclName, Channel, Condition);

// Trace a scoped cpu timing event providing a static string (const ANSICHAR* or const TCHAR*)
// as the scope name and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR("My Scoped Timer A", CpuChannel)
// Note: The event will be emitted only if both the given channel and CpuChannel is enabled.
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(NameStr, Channel) \
	TRACE_CPUPROFILER_EVENT_DECLARE(NameStr, PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__), Channel, true); \
	TRACE_CPUPROFILER_EVENT_SCOPE_USE(PREPROCESSOR_JOIN(__CpuProfilerEventSpecId, __LINE__), PREPROCESSOR_JOIN(__CpuProfilerEventScope, __LINE__), Channel, true); 

// Trace a scoped cpu timing event providing a scope name (plain text) and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(MyScopedTimer::A, CpuChannel)
// Note: Do not use this macro with a static string because, in that case, additional quotes will
//       be added around the event scope name.
// Note: The event will be emitted only if both the given channel and CpuChannel is enabled.
#define TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(Name, Channel) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(#Name, Channel)

// Trace a scoped cpu timing event providing a static string (const ANSICHAR* or const TCHAR*)
// as the scope name. It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_STR("My Scoped Timer A")
#define TRACE_CPUPROFILER_EVENT_SCOPE_STR(NameStr) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL_STR(NameStr, CpuChannel)

// Trace a scoped cpu timing event providing a scope name (plain text).
// It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE(MyScopedTimer::A)
// Note: Do not use this macro with a static string because, in that case, additional quotes will
//       be added around the event scope name.
#define TRACE_CPUPROFILER_EVENT_SCOPE(Name) \
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(Name, CpuChannel)

// Trace a scoped cpu timing event providing a dynamic string (const ANSICHAR* or const TCHAR*)
// as the scope name and a trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(*MyScopedTimerNameString, CpuChannel)
// Note: This macro has a larger overhead compared to macro that accepts a plain text name
//       or a static string. Use it only if scope name really needs to be a dynamic string.
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(Name, Channel) \
	FCpuProfilerTrace::FDynamicEventScope PREPROCESSOR_JOIN(__CpuProfilerEventScope, __LINE__)(Name, Channel, true, __FILE__, __LINE__);

// Trace a scoped cpu timing event providing a dynamic string (const ANSICHAR* or const TCHAR*)
// as the scope name. It will use the Cpu trace channel.
// Example: TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*MyScopedTimerNameString)
// Note: This macro has a larger overhead compared to macro that accepts a plain text name
//       or a static string. Use it only if scope name really needs to be a dynamic string.
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(Name) \
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(Name, CpuChannel)

struct FCpuProfilerTrace
{
    /*
     * Output cpu event definition (spec).
     * @param Name Event name
     * @param File Source filename
     * @param Line Line number in source file
     * @return Event definition id
     */
    static FORCENOINLINE uint32_t OutputEventType(const char* name, const char* file = nullptr, uint32_t line = 0);
    static FORCENOINLINE uint32_t OutputEventType2(const char* name, int name_len, const char* file = nullptr, int file_name_len = 0, uint32_t line = 0);

    /*
     * Output begin event marker for a given spec. Must always be matched with an end event.
     * @param SpecId Event definition id.
     */
    static FORCENOINLINE void OutputBeginEvent(uint32_t SpecId);
    /*
     * Output begin event marker for a dynamic event name. This is more expensive than statically known event
     * names using \ref OutputBeginEvent. Must always be matched with an end event.
     * @param Name Name of event
     * @param File Source filename
     * @param Line Line number in source file
     */
    static FORCENOINLINE void OutputBeginDynamicEvent(const char* name, const char* file = nullptr, uint32_t line = 0);
    static FORCENOINLINE void OutputBeginDynamicEvent2(const char* name, int name_len, const char* file = nullptr, int file_name_len = 0, uint32_t line = 0);
    /*
     * Output begin event marker for a dynamic event identified by an FName. This is more expensive than
     * statically known event names using \ref OutputBeginEvent, but it is faster than \ref OutputBeginDynamicEvent
     * that receives ANSICHAR* / TCHAR* name. Must always be matched with an end event.
     * @param Name Name of event
     * @param File Source filename
     * @param Line Line number in source file
     */
    //static void OutputBeginDynamicEvent(const FName& Name, const char* file = nullptr, uint32_t Line = 0);

    /*
     * Output end event marker for static or dynamic event for the currently open scope.
     */
    static FORCENOINLINE void OutputEndEvent();

    struct FEventScope
    {
        FEventScope(uint32_t InSpecId, const ::UE::Trace::FChannel& Channel, bool bCondition)
            : bEnabled(bCondition && (Channel | CpuChannel))
        {
            if (bEnabled)
            {
                OutputBeginEvent(InSpecId);
            }
        }

        ~FEventScope()
        {
            if (bEnabled)
            {
                OutputEndEvent();
            }
        }

        bool bEnabled;
    };

    struct FDynamicEventScope
    {
        FDynamicEventScope(const char* InEventName, const UE::Trace::FChannel& Channel, bool Condition, const char* File = nullptr, uint32_t Line = 0)
            : bEnabled(Condition && (Channel | CpuChannel))
        {
            if (bEnabled)
            {
                OutputBeginDynamicEvent(InEventName, File, Line);
            }
        }

        ~FDynamicEventScope()
        {
            if (bEnabled)
            {
                OutputEndEvent();
            }
        }

        bool bEnabled;
    };
};

struct FMiscTrace
{
    //static void OutputBookmarkSpec(const void* BookmarkPoint, const char* File, int32_t Line, const TCHAR* Format);
    //template <typename... Types>
    //static void OutputBookmark(const void* BookmarkPoint, Types... FormatArgs)
    //{
    //    uint8_t FormatArgsBuffer[4096];
    //    uint16_t FormatArgsSize = FFormatArgsTrace::EncodeArguments(FormatArgsBuffer, FormatArgs...);
    //    if (FormatArgsSize)
    //    {
    //        OutputBookmarkInternal(BookmarkPoint, FormatArgsSize, FormatArgsBuffer);
    //    }
    //}

    static FORCEINLINE void OutputBeginFrame(ETraceFrameType FrameType);
    static FORCEINLINE void OutputEndFrame(ETraceFrameType FrameType);
	static void OutputScreenshot(const core::string_ref& Name, uint64_t Cycle, uint32_t Width, uint32_t Height, UInt8* pngData, size_t pngSize);
	static bool ShouldTraceScreenshot();

private:
    //static void OutputBookmarkInternal(const void* BookmarkPoint, uint16_t EncodedFormatArgsSize, uint8_t* EncodedFormatArgs);
};

namespace ELogVerbosity
{
    enum Type : uint8_t
    {
        /** Not used */
        NoLogging = 0,

        /** Always prints a fatal error to console (and log file) and crashes (even if logging is disabled) */
        Fatal,

        /**
        * Prints an error to console (and log file).
        * Commandlets and the editor collect and report errors. Error messages result in commandlet failure.
        */
        Error,

        /**
        * Prints a warning to console (and log file).
        * Commandlets and the editor collect and report warnings. Warnings can be treated as an error.
        */
        Warning,

        /** Prints a message to console (and log file) */
        Display,

        /** Prints a message to a log file (does not print to console) */
        Log,

        /**
        * Prints a verbose message to a log file (if Verbose logging is enabled for the given category,
        * usually used for detailed logging)
        */
        Verbose,

        /**
        * Prints a verbose message to a log file (if VeryVerbose logging is enabled,
        * usually used for detailed logging that would otherwise spam output)
        */
        VeryVerbose,

        // Log masks and special Enum values

        All = VeryVerbose,
        NumVerbosity,
        VerbosityMask = 0xf,
        SetColor = 0x40, // not actually a verbosity, used to set the color of an output device 
        BreakOnLog = 0x80
    };
}

static_assert(ELogVerbosity::NumVerbosity - 1 < ELogVerbosity::VerbosityMask, "Bad verbosity mask.");
static_assert(!(ELogVerbosity::VerbosityMask& ELogVerbosity::BreakOnLog), "Bad verbosity mask.");

struct FLogTrace
{
    static FORCENOINLINE void OutputLogCategory(const void* LogPoint, const char* Name, ELogVerbosity::Type DefaultVerbosity);
    static FORCENOINLINE void OutputLogMessageSpec(const void* LogPoint, const void* Category, ELogVerbosity::Type Verbosity, const char* File, int32_t Line, const char* Format);

    template <typename... Types>
    FORCENOINLINE static void OutputLogMessage(const void* LogPoint, Types... FormatArgs)
    {
        UInt8 FormatArgsBuffer[3072];
        UInt16 FormatArgsSize = utrace::FFormatArgsTrace::EncodeArguments(FormatArgsBuffer, FormatArgs...);
        if (FormatArgsSize)
        {
            OutputLogMessageInternal(LogPoint, FormatArgsSize, FormatArgsBuffer);
        }
    }
    static void OutputLogMessageInternal(const void* LogPoint, uint16_t EncodedFormatArgsSize, uint8_t* EncodedFormatArgs);
};
#endif

class FTraceAuxiliary
{
public:
    enum class EConnectionType
    {
        /**
         * Connect to a trace server. Target is IP address or hostname.
         */
        Network,
        /**
         * Write to a file. Target string is filename. Absolute or relative current working directory.
         * If target is null the current date and time is used.
         */
        File,
        /**
        * Don't connect, just start tracing to memory.
        */
        None,
    };

    struct Options
    {
        /** When set, trace will not start a worker thread, instead it is updated from end frame delegate. */
        bool bNoWorkerThread = false;
        /** When set, the target file will be truncated if it already exists. */
        bool bTruncateFile = false;
    };

    /**
     * Start tracing to a target (network connection or file) with an active set of channels. If a connection is
     * already active this call does nothing.
     * @param Type Type of connection
     * @param Target String to use for connection. See /ref EConnectionType for details.
     * @param Channels Comma separated list of channels to enable. If the pointer is null the default channels will be active.
     * @param Options Optional additional tracing options.
     * @param LogCategory Log channel to output messages to. Default set to 'Core'.
     * @return True when successfully starting the trace, false if the data connection could not be made.
     */
    static bool Start(EConnectionType type, const char* target, const char* channels, Options* options = nullptr);

    /**
     * Stop tracing.
     * @return True if the trace was stopped, false if there was no data connection.
     */
    static bool Stop();

    /**
     * Pause all tracing by disabling all active channels.
     */
    static bool Pause();

    /**
     * Resume tracing by enabling all previously active channels.
     */
    static bool Resume();

    /**
     * Shut down Trace systems.
     */
    static void Shutdown();

    /**
     * Attempts to auto connect to an active trace server if an active session
     * of Unreal Insights Session Browser is running.
     */
    static void TryAutoConnect();

    /**
     *  Enable previously selected channels. This method can be called multiple times
     *  as channels can be announced on module loading.
     */
    static void EnableChannels();
};

#endif // LILITH_UNITY_UNREAL_INSIGHTS
