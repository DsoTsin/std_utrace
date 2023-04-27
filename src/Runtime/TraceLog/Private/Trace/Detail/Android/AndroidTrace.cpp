// Copyright Epic Games, Inc. All Rights Reserved.
#include "Prefix.h"
#include "Runtime/TraceLog/Public/Trace/Config.h"

#if UE_TRACE_ENABLED

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <unwind.h>

#if defined(__arm__) && defined(__GNUC__) && !defined(__clang__)
// GCC and LLVM generate slightly different frames on ARM, see
// https://llvm.org/bugs/show_bug.cgi?id=18505 - LLVM generates
// x86-compatible frame, while GCC needs adjustment.
constexpr size_t kStackFrameAdjustment = sizeof(uintptr_t);
#else
constexpr size_t kStackFrameAdjustment = 0;
#endif

namespace UE {
namespace Trace {
namespace Private {

////////////////////////////////////////////////////////////////////////////////
UPTRINT ThreadCreate(const ANSICHAR* Name, void (*Entry)())
{
	void* (*PthreadThunk)(void*) = [] (void* Param) -> void * {
		typedef void (*EntryType)(void);
		pthread_setname_np(pthread_self(), "Trace");
		(EntryType(Param))();
		return nullptr;
	};

	pthread_t ThreadHandle;
	if (pthread_create(&ThreadHandle, nullptr, PthreadThunk, reinterpret_cast<void *>(Entry)) != 0)
	{
		return 0;
	}
	return static_cast<UPTRINT>(ThreadHandle);
}

UPTRINT	ThreadCreate(const ANSICHAR* Name, void* (*entry_point)(void*), void* data, UInt32 stackSize)
{
	pthread_t ThreadHandle;
	if (pthread_create(&ThreadHandle, nullptr, entry_point, data) != 0)
	{
		return 0;
	}
	return static_cast<UPTRINT>(ThreadHandle);
}

uint32 ThreadGetId()
{
	return gettid();
}

////////////////////////////////////////////////////////////////////////////////
void ThreadSleep(uint32 Milliseconds)
{
	usleep(Milliseconds * 1000U);
}

////////////////////////////////////////////////////////////////////////////////
void ThreadJoin(UPTRINT Handle)
{
	pthread_join(static_cast<pthread_t>(Handle), nullptr);
}

////////////////////////////////////////////////////////////////////////////////
void ThreadDestroy(UPTRINT Handle)
{
	// no-op
}

//////////////////////////////   Call Stack   //////////////////////////////////

FORCEINLINE uintptr_t GetNextStackFrame(uintptr_t fp) {
    const uintptr_t* fp_addr = reinterpret_cast<const uintptr_t*>(fp);
    return fp_addr[0] - kStackFrameAdjustment;
}

FORCEINLINE uintptr_t GetStackFramePC(uintptr_t fp) {
    const uintptr_t* fp_addr = reinterpret_cast<const uintptr_t*>(fp);
    return fp_addr[1];
}

FORCEINLINE bool IsStackFrameValid(uintptr_t fp, uintptr_t prev_fp, uintptr_t stack_end) {
    // With the stack growing downwards, older stack frame must be
    // at a greater address that the current one.
    if (fp <= prev_fp) return false;
    // Assume huge stack frames are bogus.
    if (fp - prev_fp > 100000) return false;
    // Check alignment.
    if (fp & (sizeof(uintptr_t) - 1)) return false;
    if (stack_end) {
        // Both fp[0] and fp[1] must be within the stack.
        if (fp > stack_end - 2 * sizeof(uintptr_t)) return false;
        // Additional check to filter out false positives.
        if (GetStackFramePC(fp) < 32768) return false;
    }
    return true;
}

FORCEINLINE uintptr_t GetStackEnd() {
    // Bionic reads proc/maps on every call to pthread_getattr_np() when called
    // from the main thread. So we need to cache end of stack in that case to get
    // acceptable performance.
    // For all other threads pthread_getattr_np() is fast enough as it just reads
    // values from its pthread_t argument.
    static uintptr_t main_stack_end = 0;
    bool is_main_thread = getpid() == gettid();
    if (is_main_thread && main_stack_end) {
        return main_stack_end;
    }
    uintptr_t stack_begin = 0;
    size_t stack_size = 0;
    pthread_attr_t attributes;
    int error = pthread_getattr_np(pthread_self(), &attributes);
    if (!error) {
        error = pthread_attr_getstack(
                &attributes, reinterpret_cast<void**>(&stack_begin), &stack_size);
        pthread_attr_destroy(&attributes);
    }
    uintptr_t stack_end = stack_begin + stack_size;
    if (is_main_thread) {
        main_stack_end = stack_end;
    }
    return stack_end;  // 0 in case of error
}

FORCEINLINE size_t TraceStackFramePointers(void** out_trace, size_t max_depth, size_t skip_initial) {
    // Usage of __builtin_frame_address() enables frame pointers in this
    // function even if they are not enabled globally. So 'fp' will always
    // be valid.
    uintptr_t fp = reinterpret_cast<uintptr_t>(PLATFORM_RETURN_ADDRESS_POINTER()) - kStackFrameAdjustment;
    uintptr_t stack_end = GetStackEnd();
    size_t depth = 0;
    while (depth < max_depth) {
        if (skip_initial != 0) {
            skip_initial--;
        } else {
            out_trace[depth++] = reinterpret_cast<void*>(GetStackFramePC(fp));
        }
        uintptr_t next_fp = GetNextStackFrame(fp);
        if (IsStackFrameValid(next_fp, fp, stack_end)) {
            fp = next_fp;
            continue;
        }
        // Failed to find next frame.
        break;
    }
    return depth;
}

struct TraceState {
	void** current;
	void** end;
};

// Slowest
static _Unwind_Reason_Code TraceUnwind(struct _Unwind_Context* context, void* arg) {
	TraceState* state = static_cast<TraceState*>(arg);
	uintptr_t pc = _Unwind_GetIP(context);
	if (pc) {
		if (state->current == state->end) {
			return _URC_END_OF_STACK;
		} else {
			*state->current++ = reinterpret_cast<void*>(pc);
		}
	}
	return _URC_NO_REASON;
}

size_t GetCallbackFrames(void** out_trace, size_t max_depth, size_t skip_initial, UInt32& Hash, bool fastCapture)
{
    if (fastCapture)
	{
		return TraceStackFramePointers(out_trace, max_depth, skip_initial);
	}
    else
	{
		TraceState state = {out_trace, out_trace + max_depth};
		_Unwind_Backtrace(TraceUnwind, &state);
		return state.current - out_trace;
	}
}

////////////////////////////////////////////////////////////////////////////////
uint64 TimeGetFrequency()
{
	return 1000000ull;
}

////////////////////////////////////////////////////////////////////////////////
uint64 TimeGetTimestamp()
{
	// should stay in sync with FPlatformTime::Cycles64() or the timeline will be broken!
	struct timespec TimeSpec;
	clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
	return static_cast<uint64>(static_cast<uint64>(TimeSpec.tv_sec) * 1000000ULL + static_cast<uint64>(TimeSpec.tv_nsec) / 1000ULL);
}



////////////////////////////////////////////////////////////////////////////////
static bool TcpSocketSetNonBlocking(int Socket, bool bNonBlocking)
{
	int Flags = fcntl(Socket, F_GETFL, 0);
	if (Flags == -1)
	{
		return false;
	}

	Flags = bNonBlocking ? (Flags|O_NONBLOCK) : (Flags & ~O_NONBLOCK);
	return fcntl(Socket, F_SETFL, Flags) >= 0;
}

////////////////////////////////////////////////////////////////////////////////
UPTRINT TcpSocketConnect(const ANSICHAR* Host, uint16 Port)
{
	int Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0)
	{
		return 0;
	}

	sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = inet_addr(Host);
	SockAddr.sin_port = htons(Port);

	int Result = connect(Socket, (sockaddr*)&SockAddr, sizeof(SockAddr));
	if (Result < 0)
	{
		close(Socket);
		return 0;
	}

	if (!TcpSocketSetNonBlocking(Socket, false))
	{
		close(Socket);
		return 0;
	}

	return UPTRINT(Socket + 1);
}

////////////////////////////////////////////////////////////////////////////////
UPTRINT TcpSocketListen(uint16 Port)
{
	int Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket < 0)
	{
		return 0;
	}

	sockaddr_in SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = 0;
	SockAddr.sin_port = htons(Port);
	int Result = bind(Socket, reinterpret_cast<sockaddr*>(&SockAddr), sizeof(SockAddr));
	if (Result < 0)
	{
		close(Socket);
		return 0;
	}

	Result = listen(Socket, 1);
	if (Result < 0)
	{
		close(Socket);
		return 0;
	}

	if (!TcpSocketSetNonBlocking(Socket, true))
	{
		close(Socket);
		return 0;
	}

	return UPTRINT(Socket + 1);
}

////////////////////////////////////////////////////////////////////////////////
int32 TcpSocketAccept(UPTRINT Socket, UPTRINT& Out)
{
	int Inner = Socket - 1;

	Inner = accept(Inner, nullptr, nullptr);
	if (Inner < 0)
	{
		return (errno == EAGAIN || errno == EWOULDBLOCK) - 1; // 0 if would block else -1
	}

	if (!TcpSocketSetNonBlocking(Inner, false))
	{
		close(Inner);
		return 0;
	}

	Out = UPTRINT(Inner + 1);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
bool TcpSocketHasData(UPTRINT Socket)
{
	int Inner = Socket - 1;
	fd_set FdSet;
	FD_ZERO(&FdSet);
	FD_SET(Inner, &FdSet);
	timeval TimeVal = {};
	return (select(Inner + 1, &FdSet, nullptr, nullptr, &TimeVal) != 0);
}



////////////////////////////////////////////////////////////////////////////////
bool IoWrite(UPTRINT Handle, const void* Data, uint32 Size)
{
	int Inner = int(Handle) - 1;
	return write(Inner, Data, Size) == Size;
}

////////////////////////////////////////////////////////////////////////////////
int32 IoRead(UPTRINT Handle, void* Data, uint32 Size)
{
	int Inner = int(Handle) - 1;
	return read(Inner, Data, Size);
}

////////////////////////////////////////////////////////////////////////////////
void IoClose(UPTRINT Handle)
{
	int Inner = int(Handle) - 1;
	close(Inner);
}



////////////////////////////////////////////////////////////////////////////////
UPTRINT FileOpen(const ANSICHAR* Path)
{
	int Flags = O_CREAT|O_WRONLY|O_TRUNC;
	int Mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

	int Out = open(Path, Flags, Mode);
	if (Out < 0)
	{
		return 0;
	}

	return UPTRINT(Out + 1);
}

} // namespace Private
} // namespace Trace
} // namespace UE

#endif // UE_TRACE_ENABLED
