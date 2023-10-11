#ifndef __utrace_h__
#define __utrace_h__

#ifdef BUILD_UTRACE_LIB
#ifdef _MSC_VER
#define UTRACE_API __declspec(dllexport)
#else
#define UTRACE_API __attribute__((visibility("default")))
#endif
#else
#define UTRACE_API   
#endif

#define MAKE_UTRACE_STR(x) { x, (utrace_u32)__builtin_strlen(x) }

#include <stdint.h>

typedef int64_t	    utrace_i64;
typedef uint64_t	utrace_u64;
typedef uint32_t	utrace_u32;
typedef uint32_t	utrace_u16;
typedef uint8_t		utrace_u8;

#ifdef __cplusplus
extern "C" {
#endif
	typedef enum utrace_target_type {
		UTRACE_TARGET_TYPE_UNKNOWN,
		UTRACE_TARGET_TYPE_GAME,
		UTRACE_TARGET_TYPE_SERVER,
		UTRACE_TARGET_TYPE_CLIENT,
		UTRACE_TARGET_TYPE_EDITOR,
		UTRACE_TARGET_TYPE_PROGRAM,
	} utrace_target_type;

	typedef enum utrace_build_configuration {
		UTRACE_BUILD_CONFIGURATION_UNKNOWN,
		UTRACE_BUILD_CONFIGURATION_DEBUG,
		UTRACE_BUILD_CONFIGURATION_DEBUG_GAME,
		UTRACE_BUILD_CONFIGURATION_DEVELOPMENT,
		UTRACE_BUILD_CONFIGURATION_SHIPPING,
		UTRACE_BUILD_CONFIGURATION_TEST,
	} utrace_build_configuration;

    typedef enum utrace_mem_heap {
        UTRACE_MEM_HEAP_SYSTEM,
		UTRACE_MEM_HEAP_VIDEO,
		UTRACE_MEM_HEAP_ENGINE,
		UTRACE_MEM_HEAP_LIBC,
		UTRACE_MEM_HEAP_PROGRAM,
    } utrace_mem_heap;

    typedef enum utrace_mem_heap_flag {
        UTRACE_MEM_HEAP_FLAG_NONE = 0,
        UTRACE_MEM_HEAP_FLAG_ROOT = 1,
        UTRACE_MEM_HEAP_FLAG_NEVER_FREE = 2,
    } utrace_mem_heap_flag;

    typedef enum utrace_log_type
    {
        /** Not used */
        UTRACE_LOG_TYPE_NO_LOGGING = 0,

        /** Always prints a fatal error to console (and log file) and crashes (even if logging is disabled) */
        UTRACE_LOG_TYPE_FATAL,

        /**
        * Prints an error to console (and log file).
        * Commandlets and the editor collect and report errors. Error messages result in commandlet failure.
        */
        UTRACE_LOG_TYPE_ERROR,

        /**
        * Prints a warning to console (and log file).
        * Commandlets and the editor collect and report warnings. Warnings can be treated as an error.
        */
        UTRACE_LOG_TYPE_WARNING,

        /** Prints a message to console (and log file) */
        UTRACE_LOG_TYPE_DISPLAY,

        /** Prints a message to a log file (does not print to console) */
        UTRACE_LOG_TYPE_LOG,

        /**
        * Prints a verbose message to a log file (if Verbose logging is enabled for the given category,
        * usually used for detailed logging)
        */
        UTRACE_LOG_TYPE_VERBOSE,

        /**
        * Prints a verbose message to a log file (if VeryVerbose logging is enabled,
        * usually used for detailed logging that would otherwise spam output)
        */
        UTRACE_LOG_TYPE_VERY_VERBOSE,

        // Log masks and special Enum values

        UTRACE_LOG_TYPE_ALL = UTRACE_LOG_TYPE_VERY_VERBOSE,
        UTRACE_LOG_TYPE_NUM_VERBOSITY,
        UTRACE_LOG_TYPE_VERBOSITY_MASK = 0xf,
        UTRACE_LOG_TYPE_SET_COLOR = 0x40, // not actually a verbosity, used to set the color of an output device 
        UTRACE_LOG_TYPE_BREAK_ON_LOG = 0x80
    } utrace_log_type;

    typedef enum utrace_counter_type
    {
        UTRACE_COUNTER_TYPE_INT = 0,
        UTRACE_COUNTER_TYPE_FLOAT = 1,
    } utrace_counter_type;
    
    typedef enum utrace_counter_display_hint
    {
        UTRACE_COUNTER_DISPLAY_HINT_NONE = 0,
        UTRACE_COUNTER_DISPLAY_HINT_MEMORY = 1,
    } utrace_counter_display_hint;

	typedef struct utrace_string {
		const char* str;
		utrace_u32 len;
	} utrace_string;

	UTRACE_API void utrace_initialize_main_thread(
		utrace_string appName,
		utrace_string buildVersion,
		utrace_string commandLine,
		utrace_string branchName,
		utrace_u32 changeList,
		utrace_target_type target_type,
		utrace_build_configuration build_configuration,
		utrace_u32 tailBytes
	);

	UTRACE_API void utrace_try_connect();
	UTRACE_API utrace_u64 utrace_get_timestamp();

	UTRACE_API int 	utrace_screenshot_enabled();
	UTRACE_API void utrace_screenshot_write(
		utrace_string shotName,
		utrace_u64 timestamp, 
		utrace_u32 width, utrace_u32 height, 
		unsigned char* imagePNGData, utrace_u32 imagePNGSize
	);

	UTRACE_API void utrace_register_thread(
		const char* name, 
		utrace_u32 sysId, 
		int sortHint
	);
	
	UTRACE_API void utrace_cpu_begin_name(const char* name, const char* file, int line);
	UTRACE_API void utrace_cpu_begin_name2(const char* name, int name_len, const char* file_name, int file_name_len, int line);
	UTRACE_API void utrace_cpu_end();

	UTRACE_API void	utrace_emit_render_frame(int is_begin);
	UTRACE_API void	utrace_emit_game_frame(int is_begin);
	
	UTRACE_API int 	utrace_log_enabled();
	UTRACE_API void	utrace_log_decl_category(const void* category, utrace_string name, utrace_log_type type);
	UTRACE_API void	utrace_log_decl_spec(const void* logPoint, const void* category, utrace_log_type verbosity, utrace_string file, int line, const char* format);
	UTRACE_API void	utrace_log_write_buffer(const void* logPoint, utrace_u16 encoded_arg_size, utrace_u8* encoded_arg_buffer);

	UTRACE_API void utrace_file_begin_open(utrace_string path);
	UTRACE_API void utrace_file_fail_open(utrace_string path);
	UTRACE_API void utrace_file_begin_reopen(utrace_u64 oldHandle);
	UTRACE_API void utrace_file_end_reopen(utrace_u64 newHandle);
	UTRACE_API void utrace_file_end_open(utrace_u64 handle);
	UTRACE_API void utrace_file_begin_close(utrace_u64 handle);
	UTRACE_API void utrace_file_end_close(utrace_u64 handle);
	UTRACE_API void utrace_file_fail_close(utrace_u64 handle);
    UTRACE_API void utrace_file_begin_read(utrace_u64 readHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size);
    UTRACE_API void utrace_file_end_read(utrace_u64 readHandle, utrace_u64 sizeRead);
    UTRACE_API void utrace_file_begin_write(utrace_u64 writeHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size);
    UTRACE_API void utrace_file_end_write(utrace_u64 writeHandle, utrace_u64 sizeWritten);
	
    UTRACE_API utrace_u16 utrace_counter_init(utrace_string name, utrace_counter_type type, utrace_counter_display_hint hint);
    UTRACE_API void utrace_counter_set_int(utrace_u16 counter_id, utrace_i64 value);
    UTRACE_API void utrace_counter_set_float(utrace_u16 counter_id, double value);
    
    UTRACE_API void utrace_mem_init();
    
    UTRACE_API utrace_u32 utrace_mem_alloc(utrace_u64 address, utrace_u64 size, utrace_u32 alignment, utrace_u32 rootHeap);
    UTRACE_API utrace_u32 utrace_mem_free(utrace_u64 address, utrace_u32 rootHeap);
    UTRACE_API void utrace_mem_realloc_free(utrace_u64 address, utrace_u32 rootHeap);
    UTRACE_API void utrace_mem_realloc_alloc(utrace_u64 address, utrace_u64 newSize, utrace_u32 alignment, utrace_u32 rootHeap);
    UTRACE_API void utrace_mem_init_tags();
    UTRACE_API int	utrace_mem_announce_custom_tag(int tag, int parentTag, utrace_string display);
    UTRACE_API int	utrace_mem_name_tag(utrace_string tagName);
    UTRACE_API int	utrace_mem_get_active_tag();
    UTRACE_API void	utrace_mem_mark_object_name(void* address, utrace_string objName);
    UTRACE_API utrace_u32 utrace_mem_heap_spec(utrace_u32 parentId, utrace_string name, utrace_u32 flags);
    UTRACE_API utrace_u32 utrace_mem_root_heap_spec(utrace_string name, utrace_u32 flags);
    UTRACE_API void utrace_mem_mark_alloc_as_heap(utrace_u64 addr, utrace_u32 heap, utrace_u32 flags);
    UTRACE_API void utrace_mem_unmark_alloc_as_heap(utrace_u64 addr, utrace_u32 heap);
	// Mmap or Virtual alloc
	UTRACE_API void utrace_mem_trace_virtual_alloc(int traced);
    UTRACE_API void utrace_mem_scope_ctor(void* ptr, utrace_string name, utrace_u8 active);
    UTRACE_API void utrace_mem_scope_dtor(void* ptr);
    
    UTRACE_API void utrace_mem_scope_ptr_ctor(void* ptr, void* addr);
    UTRACE_API void utrace_mem_scope_ptr_dtor(void* ptr);

	UTRACE_API void	utrace_module_init(utrace_string fmt, unsigned char shift);
	UTRACE_API void	utrace_module_load(utrace_string name, utrace_u64 base, utrace_u32 size, const utrace_u8* imageIdData, utrace_u32 imageIdLen);
	UTRACE_API void	utrace_module_load2(utrace_string name, utrace_u64 base, utrace_u32 size, utrace_u64 fileOffset, const utrace_u8* imageIdData, utrace_u32 imageIdLen);
	UTRACE_API void	utrace_module_unload(utrace_u64 base);

	UTRACE_API void	utrace_vulkan_set_render_pass_name(utrace_u64 render_pass, utrace_string name);
	UTRACE_API void	utrace_vulkan_set_frame_buffer_name(utrace_u64 frame_buffer, utrace_string name);
	UTRACE_API void	utrace_vulkan_set_command_buffer_name(utrace_u64 command_buffer, utrace_string name);

	UTRACE_API utrace_u32 utrace_current_callstack_id();

#ifdef __cplusplus
}

#include <string.h>
#include <assert.h>

namespace utrace {
#if __cplusplus > 201103L || (defined(_MSC_VER) && _MSC_VER > 1900)
enum EMemoryTraceRootHeap : utrace_u8 {
    SystemMemory,       // Custom virtual allocations
    VideoMemory,        // VRAM, allocated by GPU
    UnityEngineMemory,  // Allocated by Engine
    LibCMemory,         // C malloced memory
    ProgramMemory,      // Allocated by linker
    EndHardcoded = ProgramMemory,
    EndReserved = 15
};

////////////////////////////////////////////////////////////////////////////////
// These values are traced. Do not modify existing values in order to maintain
// compatibility.
enum class EMemoryTraceHeapFlags : utrace_u16 {
    None = 0,
    Root = 1 << 0,
    NeverFrees = 1 << 1, // The heap doesn't free (e.g. linear allocator)
};

////////////////////////////////////////////////////////////////////////////////
// These values are traced. Do not modify existing values in order to maintain
// compatibility.
enum class EMemoryTraceHeapAllocationFlags : utrace_u8 {
    None = 0,
    Heap = 1 << 0, // Is a heap, can be used to unmark alloc as heap.
};
#endif

struct Scope {
	Scope(const char* name, int nameLen, const char* filename, int filenameLen, int fileLine) {
		utrace_cpu_begin_name2(name, nameLen, filename, filenameLen, fileLine);
	}
	Scope(const char* name, const char* filename, int fileLine) {
		utrace_cpu_begin_name(name, filename, fileLine);
	}
	~Scope() {
		utrace_cpu_end();
	}
};

template<typename ValueType, utrace_counter_type CounterType, void SetFunc(utrace_u16, ValueType)>
class TCounter {
public:
	TCounter(utrace_string InCounterName, utrace_counter_display_hint InCounterDisplayHint)
		: Value(0)
		, CounterName(InCounterName)
		, CounterId(0)
		, CounterDisplayHint(InCounterDisplayHint)
	{
		CounterId = utrace_counter_init(InCounterName, CounterType, CounterDisplayHint);
	}

	const ValueType& Get() const
	{
		return Value;
	}

	void Set(ValueType InValue)
	{
		if (Value != InValue)
		{
			Value = InValue;
			SetFunc(CounterId, Value);
		}
	}

	void Add(ValueType InValue)
	{
		if (InValue != 0)
		{
			Value += InValue;
			SetFunc(CounterId, Value);
		}
	}

	void Subtract(ValueType InValue)
	{
		if (InValue != 0)
		{
			Value -= InValue;
			SetFunc(CounterId, Value);
		}
	}

	void Increment()
	{
		++Value;
		SetFunc(CounterId, Value);
	}
		
	void Decrement()
	{
		--Value;
		SetFunc(CounterId, Value);
	}

private:
	ValueType					Value;
	utrace_string				CounterName;
	utrace_u16					CounterId;
	utrace_counter_display_hint CounterDisplayHint;
};

using FCounterInt = TCounter<utrace_i64, UTRACE_COUNTER_TYPE_INT, utrace_counter_set_int>;
using FCounterFloat = TCounter<double, UTRACE_COUNTER_TYPE_FLOAT, utrace_counter_set_float>;

#if __cplusplus > 201103L || (defined(_MSC_VER) && _MSC_VER > 1900)
struct FFormatArgsTrace
{
    enum EFormatArgTypeCode
    {
        FormatArgTypeCode_CategoryBitShift = 6,
        FormatArgTypeCode_SizeBitMask = (1 << FormatArgTypeCode_CategoryBitShift) - 1,
        FormatArgTypeCode_CategoryBitMask = ~FormatArgTypeCode_SizeBitMask,
        FormatArgTypeCode_CategoryInteger = 1 << FormatArgTypeCode_CategoryBitShift,
        FormatArgTypeCode_CategoryFloatingPoint = 2 << FormatArgTypeCode_CategoryBitShift,
        FormatArgTypeCode_CategoryString = 3 << FormatArgTypeCode_CategoryBitShift,
    };

    template <int BufferSize, typename... Types>
    static uint16_t EncodeArguments(uint8_t(&Buffer)[BufferSize], Types... FormatArgs)
    {
        static_assert(BufferSize < 65536, "Maximum buffer size of 16 bits exceeded");
        uint64_t FormatArgsCount = sizeof...(FormatArgs);
        if (FormatArgsCount >= 256)
        {
            // Nope
            return 0;
        }
        uint64_t FormatArgsSize = 1 + FormatArgsCount + GetArgumentsEncodedSize(FormatArgs...);
        if (FormatArgsSize > BufferSize)
        {
            // Nope
            return 0;
        }
        uint8_t* TypeCodesBufferPtr = Buffer;
        *TypeCodesBufferPtr++ = uint8_t(FormatArgsCount);
        uint8_t* PayloadBufferPtr = TypeCodesBufferPtr + FormatArgsCount;
        EncodeArgumentsInternal(TypeCodesBufferPtr, PayloadBufferPtr, FormatArgs...);
        assert(PayloadBufferPtr - Buffer == FormatArgsSize);
        return (uint16_t)FormatArgsSize;
    }

private:

    template <typename T>
    constexpr static int GetArgumentEncodedSize(T Argument)
    {
        return (int)sizeof(T);
    }

    static int GetArgumentEncodedSize(const char* Argument)
    {
        if (Argument != nullptr)
        {
            return int(strlen(Argument) + 1);
        }
        else
        {
            return 1;
        }
    }

    constexpr static int GetArgumentsEncodedSize()
    {
        return 0;
    }

    template <typename T, typename... Types>
    static int GetArgumentsEncodedSize(T Head, Types... Tail)
    {
        return GetArgumentEncodedSize(Head) + GetArgumentsEncodedSize(Tail...);
    }

    template <typename T>
    static void EncodeArgumentInternal(uint8_t*& TypeCodesPtr, uint8_t*& PayloadPtr, T Argument)
    {
        *TypeCodesPtr++ = FormatArgTypeCode_CategoryInteger | sizeof(T);

#if PLATFORM_SUPPORTS_UNALIGNED_LOADS
        * reinterpret_cast<T*>(PayloadPtr) = Argument;
#else
        // For ARM targets, it's possible that using __packed here would be preferable
        // but I have not checked the codegen -- it's possible that the compiler generates
        // the same code for this fixed size memcpy
        memcpy(PayloadPtr, &Argument, sizeof Argument);
#endif

        PayloadPtr += sizeof(T);
    }

    static void EncodeArgumentInternal(uint8_t*& TypeCodesPtr, uint8_t*& PayloadPtr, float Argument)
    {
        *TypeCodesPtr++ = FormatArgTypeCode_CategoryFloatingPoint | sizeof(float);

#if PLATFORM_SUPPORTS_UNALIGNED_LOADS
        * reinterpret_cast<T*>(PayloadPtr) = Argument;
#else
        // For ARM targets, it's possible that using __packed here would be preferable
        // but I have not checked the codegen -- it's possible that the compiler generates
        // the same code for this fixed size memcpy
        memcpy(PayloadPtr, &Argument, sizeof Argument);
#endif

        PayloadPtr += sizeof(float);
    }

    static void EncodeArgumentInternal(uint8_t*& TypeCodesPtr, uint8_t*& PayloadPtr, const char* Argument)
    {
        *TypeCodesPtr++ = FormatArgTypeCode_CategoryString | 1;
        if (Argument != nullptr)
        {
            uint16_t Length = (uint16_t)((strlen(Argument) + 1));
            memcpy(PayloadPtr, Argument, Length);
            PayloadPtr += Length;
        }
        else
        {
            char Terminator{ 0 };
            memcpy(PayloadPtr, &Terminator, 1);
            PayloadPtr += 1;
        }
    }

    constexpr static void EncodeArgumentsInternal(uint8_t*& TypeCodesPtr, uint8_t*& PayloadPtr)
    {
    }

    template <typename T, typename... Types>
    static void EncodeArgumentsInternal(uint8_t*& ArgDescriptorsPtr, uint8_t*& ArgPayloadPtr, T Head, Types... Tail)
    {
        EncodeArgumentInternal(ArgDescriptorsPtr, ArgPayloadPtr, Head);
        EncodeArgumentsInternal(ArgDescriptorsPtr, ArgPayloadPtr, Tail...);
    }
};

template <typename... Types>
void log_write(const void* LogPoint, Types... FormatArgs)
{
	utrace_u8 FormatArgsBuffer[3072];
	utrace_u16 FormatArgsSize = FFormatArgsTrace::EncodeArguments(FormatArgsBuffer, FormatArgs...);
	if (FormatArgsSize)
	{
		utrace_log_write_buffer(LogPoint, FormatArgsSize, FormatArgsBuffer);
	}
}
#endif // c++ 14
}

#define __PP_CONCAT_Y(A_, B_) A_##B_
#define __PP_CONCAT_X(A_, B_) __PP_CONCAT_Y(A_, B_)
#define UTRACE_AUTO(x) utrace::Scope __PP_CONCAT_X(__$utrace, __LINE__)((x), __builtin_strlen(x), __FILE__, __builtin_strlen(__FILE__), __LINE__)

#define __TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, CounterType, CounterDisplayHint) \
	static utrace::CounterType __PP_CONCAT_X(__TraceCounter, __LINE__)(MAKE_UTRACE_STR(CounterDisplayName), CounterDisplayHint);

#define TRACE_INT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, UTRACE_COUNTER_DISPLAY_HINT_NONE) \
	__PP_CONCAT_X(__TraceCounter, __LINE__).Set(Value);

#define TRACE_FLOAT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterFloat, UTRACE_COUNTER_DISPLAY_HINT_NONE) \
	__PP_CONCAT_X(__TraceCounter, __LINE__).Set(Value);

#define TRACE_MEMORY_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, UTRACE_COUNTER_DISPLAY_HINT_MEMORY) \
	__PP_CONCAT_X(__TraceCounter, __LINE__).Set(Value);

#define TRACE_DECLARE_INT_COUNTER(CounterName, CounterDisplayName) \
	utrace::FCounterInt __PP_CONCAT_X(__GTraceCounter, CounterName)(MAKE_UTRACE_STR(CounterDisplayName), UTRACE_COUNTER_DISPLAY_HINT_NONE);

#define TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName) \
	extern utrace::FCounterInt __PP_CONCAT_X(__GTraceCounter, CounterName);

#define TRACE_DECLARE_FLOAT_COUNTER(CounterName, CounterDisplayName) \
	utrace::FCounterFloat __PP_CONCAT_X(__GTraceCounter, CounterName)(MAKE_UTRACE_STR(CounterDisplayName), UTRACE_COUNTER_DISPLAY_HINT_NONE);

#define TRACE_DECLARE_FLOAT_COUNTER_EXTERN(CounterName) \
	extern utrace::FCounterFloat __PP_CONCAT_X(__GTraceCounter, CounterName);

#define TRACE_DECLARE_MEMORY_COUNTER(CounterName, CounterDisplayName) \
	utrace::FCounterInt __PP_CONCAT_X(__GTraceCounter, CounterName)(MAKE_UTRACE_STR(CounterDisplayName), UTRACE_COUNTER_DISPLAY_HINT_MEMORY);

#define TRACE_DECLARE_MEMORY_COUNTER_EXTERN(CounterName) \
	TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName)

#define TRACE_COUNTER_SET(CounterName, Value) \
	__PP_CONCAT_X(__GTraceCounter, CounterName).Set(Value);

#define TRACE_COUNTER_ADD(CounterName, Value) \
	__PP_CONCAT_X(__GTraceCounter, CounterName).Add(Value);

#define TRACE_COUNTER_SUBTRACT(CounterName, Value) \
	__PP_CONCAT_X(__GTraceCounter, CounterName).Subtract(Value);

#define TRACE_COUNTER_INCREMENT(CounterName) \
	__PP_CONCAT_X(__GTraceCounter, CounterName).Increment();

#define TRACE_COUNTER_DECREMENT(CounterName) \
	__PP_CONCAT_X(__GTraceCounter, CounterName).Decrement();

#endif

#endif