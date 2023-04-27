#include "Backtracer.h"

#include "Runtime/TraceLog/Public/Trace/Trace.h"
#include "Runtime/TraceLog/Public/Trace/Trace.inl"
#include "Runtime/TraceLog/Public/Trace/Detail/EventNode.h"
#include "Runtime/TraceLog/Public/Trace/Detail/Important/ImportantLogScope.h"
#include "Runtime/TraceLog/Public/Trace/Detail/LogScope.h"

UE_TRACE_CHANNEL_DEFINE(CallstackChannel)

UE_TRACE_EVENT_BEGIN(Memory, CallstackSpec, NoSync)
	UE_TRACE_EVENT_FIELD(UInt32, CallstackId)
	UE_TRACE_EVENT_FIELD(UInt64[], Frames)
UE_TRACE_EVENT_END()

#ifndef UE_CALLSTACK_TRACE_FULL_CALLSTACKS
	#define UE_CALLSTACK_TRACE_FULL_CALLSTACKS 0
#endif

namespace UE
{
namespace Trace
{
class FCallstackTracer
{
public:
	struct FBacktraceEntry
	{
		UInt64	Hash = 0;
		UInt32	FrameCount = 0;
		void** 	Frames;
	};

	FCallstackTracer()
		: CallstackIdCounter(1) // 0 is reserved for "unknown callstack"
	{
	}
		
	UInt32 AddCallstack(const FBacktraceEntry& Entry)
	{
		bool bAlreadyAdded = false;

		// Our set implementation doesn't allow for zero entries (zero represents an empty element
		// in the hash table), so if we get one due to really bad luck in our 64-bit Id calculation,
		// treat it as a "1" instead, for purposes of tracking if we've seen that callstack.
		const UInt64 Hash = Entry.Hash > 1ull ? Entry.Hash : 1ull;
		UInt32 Id; 
		KnownSet.Find(Hash, &Id, &bAlreadyAdded);
		if (!bAlreadyAdded)
		{
			Id = CallstackIdCounter.fetch_add(1, std::memory_order_relaxed);
			// On the first callstack reserve memory up front
			if (Id == 1)
			{
				KnownSet.Reserve(InitialReserveCount);
			}
#if !UE_CALLSTACK_TRACE_RESERVE_GROWABLE
			// If configured as not growable, start returning unknown id's when full.
			if (Id >= InitialReserveCount)
			{
				return 0;
			}
#endif
			KnownSet.Emplace(Hash, Id);

			UInt64 Stacks[CALLSTACK_BUFFERSIZE];
			for(UInt32 i = 0; i < Entry.FrameCount; i++)
			{
				Stacks[i] = (UInt64)Entry.Frames[i];
			}

			UE_TRACE_LOG(Memory, CallstackSpec, CallstackChannel)
				<< CallstackSpec.CallstackId(Id)
				<< CallstackSpec.Frames(Stacks, Entry.FrameCount);
		}

		return Id;
	}
private:
	struct FEncounteredCallstackSetEntry
	{
		std::atomic_uint64_t Key;
		std::atomic_uint32_t Value;

		inline UInt64 GetKey() const { return Key.load(std::memory_order_relaxed); }
		inline UInt32 GetValue() const { return Value.load(std::memory_order_relaxed); }
		inline bool IsEmpty() const { return Key.load(std::memory_order_relaxed) == 0; }
		inline void SetKeyValue(UInt64 InKey, UInt32 InValue)
		{
			Value.store(InValue, std::memory_order_release);
			Key.store(InKey, std::memory_order_relaxed);
		}
		static inline UInt32 KeyHash(UInt64 Key) { return static_cast<UInt32>(Key); }
		static inline void ClearEntries(FEncounteredCallstackSetEntry* Entries, SInt32 EntryCount)
		{
			memset(Entries, 0, EntryCount * sizeof(FEncounteredCallstackSetEntry));
		}
	};
	typedef TGrowOnlyLockFreeHash<FEncounteredCallstackSetEntry, UInt64, UInt32> FEncounteredCallstackSet;

	constexpr static UInt32 InitialReserveBytes = UE_CALLSTACK_TRACE_RESERVE_MB * 1024 * 1024;
	constexpr static UInt32 InitialReserveCount = InitialReserveBytes / sizeof(FEncounteredCallstackSetEntry);
		
	FEncounteredCallstackSet 	KnownSet;
	std::atomic_uint32_t		CallstackIdCounter;
};

////////////////////////////////////////////////////////////////////////////////
FBacktracer* FBacktracer::Instance = nullptr;
static bool GFullBacktraces = UE_CALLSTACK_TRACE_FULL_CALLSTACKS;
static bool GModulesAreInitialized = false;

////////////////////////////////////////////////////////////////////////////////
FBacktracer::FBacktracer()
#if BACKTRACE_LOCK_FREE
	: FunctionLookups(InMalloc)
#endif
{
#if BACKTRACE_LOCK_FREE
	FunctionLookups.Reserve(512 * 1024);		// 4 MB
#endif
	ModulesCapacity = 8;
	ModulesNum = 0;
	Modules = (FModule*)malloc(sizeof(FModule) * ModulesCapacity);

	Instance = this;
#if 0
	CallstackTracer = (FCallstackTracer*) alignalloc(sizeof(FCallstackTracer), alignof(FCallstackTracer));
	CallstackTracer = new(CallstackTracer) FCallstackTracer();
#endif
}

////////////////////////////////////////////////////////////////////////////////
FBacktracer::~FBacktracer()
{
#if 0
	TArrayView<FModule> ModulesView(Modules, ModulesNum);
	for (FModule& Module : ModulesView)
	{
		Malloc->Free(Module.Functions);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
FBacktracer* FBacktracer::Get()
{
	return Instance;
}

////////////////////////////////////////////////////////////////////////////////
void FBacktracer::AddModule(UPTRINT ModuleBase, const TCHAR* Name)
{
#if 0
	const auto* DosHeader = (IMAGE_DOS_HEADER*)ModuleBase;
	const auto* NtHeader = (IMAGE_NT_HEADERS*)(ModuleBase + DosHeader->e_lfanew);
	const IMAGE_FILE_HEADER* FileHeader = &(NtHeader->FileHeader);

	if(!GFullBacktraces)
	{
		int32 NameLen = FCString::Strlen(Name);
		if (!(NameLen > 4 && FCString::Strcmp(Name + NameLen - 4, TEXT(".exe")) == 0) && // we want always load an .exe file
			(FCString::Strfind(Name, TEXT("Binaries")) == nullptr || FCString::Strfind(Name, TEXT("ThirdParty")) != nullptr))
		{
			return;
		}
	}

	uint32 NumSections = FileHeader->NumberOfSections;
	const auto* Sections = (IMAGE_SECTION_HEADER*)(UPTRINT(&(NtHeader->OptionalHeader)) + FileHeader->SizeOfOptionalHeader);

	// Find ".pdata" section
	UPTRINT PdataBase = 0;
	UPTRINT PdataEnd = 0;
	for (uint32 i = 0; i < NumSections; ++i)
	{
		const IMAGE_SECTION_HEADER* Section = Sections + i;
		if (*(uint64*)(Section->Name) == 0x61'74'61'64'70'2eull) // Sections names are eight bytes and zero padded. This constant is '.pdata'
		{
			PdataBase = ModuleBase + Section->VirtualAddress;
			PdataEnd = PdataBase + Section->SizeOfRawData;
			break;
		}
	}

	if (PdataBase == 0)
	{
		return;
	}

	// Count the number of functions. The assumption here is that if we have got this far then there is at least one function
	uint32 NumFunctions = uint32(PdataEnd - PdataBase) / sizeof(RUNTIME_FUNCTION);
	if (NumFunctions == 0)
	{
		return;
	}

	const auto* FunctionTables = (RUNTIME_FUNCTION*)PdataBase;
	do
	{
		const RUNTIME_FUNCTION* Function = FunctionTables + NumFunctions - 1;
		if (uint32(Function->BeginAddress) < uint32(Function->EndAddress))
		{
			break;
		}

		--NumFunctions;
	}
	while (NumFunctions != 0);
		
	// Allocate some space for the module's function-to-frame-size table
	auto* OutTable = (FFunction*)Malloc->Malloc(sizeof(FFunction) * NumFunctions);
	FFunction* OutTableCursor = OutTable;

	// Extract frame size for each function from pdata's unwind codes.
	uint32 NumFpFuncs = 0;
	for (uint32 i = 0; i < NumFunctions; ++i)
	{
		const RUNTIME_FUNCTION* FunctionTable = FunctionTables + i;

		UPTRINT UnwindInfoAddr = ModuleBase + FunctionTable->UnwindInfoAddress;
		const auto* UnwindInfo = (FUnwindInfo*)UnwindInfoAddr;

		if (UnwindInfo->Version != 1)
		{
			/* some v2s have been seen in msvc. Always seem to be assembly
			 * routines (memset, memcpy, etc) */
			continue;
		}

		int32 FpInfo = 0;
		int32 RspBias = 0;

#if BACKTRACE_DBGLVL >= 2
		uint32 PrologVerify = UnwindInfo->PrologBytes;
#endif

		const auto* Code = (FUnwindCode*)(UnwindInfo + 1);
		const auto* EndCode = Code + UnwindInfo->NumUnwindCodes;
		while (Code < EndCode)
		{
#if BACKTRACE_DBGLVL >= 2
			if (Code->PrologOffset > PrologVerify)
			{
				PLATFORM_BREAK();
			}
			PrologVerify = Code->PrologOffset;
#endif

			switch (Code->OpCode)
			{
			case UWOP_PUSH_NONVOL:
				RspBias += 8;
				Code += 1;
				break;

			case UWOP_ALLOC_LARGE:
				if (Code->OpInfo)
				{
					RspBias += *(uint32*)(Code->Params);
					Code += 3;
				}
				else
				{
					RspBias += Code->Params[0] * 8;
					Code += 2;
				}
				break;

			case UWOP_ALLOC_SMALL:
				RspBias += (Code->OpInfo * 8) + 8;
				Code += 1;
				break;

			case UWOP_SET_FPREG:
				// Function will adjust RSP (e.g. through use of alloca()) so it
				// uses a frame pointer register. There's instructions like;
				//
				//   push FRAME_REG
				//   lea FRAME_REG, [rsp + (FRAME_RSP_BIAS * 16)]
				//   ...
				//   add rsp, rax
				//   ...
				//   sub rsp, FRAME_RSP_BIAS * 16
				//   pop FRAME_REG
				//   ret
				//
				// To recover the stack frame we would need to track non-volatile
				// registers which adds a lot of overhead for a small subset of
				// functions. Instead we'll end backtraces at these functions.


				// MSB is set to detect variable sized frames that we can't proceed
				// past when back-tracing.
				NumFpFuncs++;
				FpInfo |= 0x80000000 | (uint32(UnwindInfo->FrameReg) << 27) | (uint32(UnwindInfo->FrameRspBias) << 23);
				Code += 1;
				break;

			case UWOP_PUSH_MACHFRAME:
				RspBias = Code->OpInfo ? 48 : 40;
				Code += 1;
				break;

			case UWOP_SAVE_NONVOL:		Code += 2; break; /* saves are movs instead of pushes */
			case UWOP_SAVE_NONVOL_FAR:	Code += 3; break;
			case UWOP_SAVE_XMM128:		Code += 2; break;
			case UWOP_SAVE_XMM128_FAR:	Code += 3; break;

			default:
#if BACKTRACE_DBGLVL >= 2
				PLATFORM_BREAK();
#endif
				break;
			}
		}

		// "Chained" simply means that multiple RUNTIME_FUNCTIONs pertains to a
		// single actual function in the .text segment.
		bool bIsChained = (UnwindInfo->Flags & UNW_FLAG_CHAININFO);

		RspBias /= sizeof(void*);	// stack push/popds in units of one machine word
		RspBias += !bIsChained;		// and one extra push for the ret address
		RspBias |= FpInfo;			// pack in details about possible frame pointer

		if (bIsChained)
		{
			OutTableCursor[-1].RspBias += RspBias;
#if BACKTRACE_DBGLVL >= 2
			OutTableCursor[-1].Size += (FunctionTable->EndAddress - FunctionTable->BeginAddress);
#endif
		}
		else
		{
			*OutTableCursor = {
				FunctionTable->BeginAddress,
				RspBias,
#if BACKTRACE_DBGLVL >= 2
				FunctionTable->EndAddress - FunctionTable->BeginAddress,
				UnwindInfo,
#endif
			};

			++OutTableCursor;
		}
	}

	UPTRINT ModuleSize = NtHeader->OptionalHeader.SizeOfImage;
	ModuleSize += 0xffff; // to align up to next 64K page. it'll get shifted by AddressToId()

	FModule Module = {
		AddressToId(ModuleBase),
		AddressToId(ModuleSize),
		uint32(UPTRINT(OutTableCursor - OutTable)),
#if BACKTRACE_DBGLVL >= 1
		uint16(NumFpFuncs),
#endif
		OutTable,
	};

	{
		FScopeLock _(&Lock);

		TArrayView<FModule> ModulesView(Modules, ModulesNum);
		int32 Index = Algo::UpperBound(ModulesView, Module.Id, FIdPredicate());

		if (ModulesNum + 1 > ModulesCapacity)
		{
			ModulesCapacity += 8;
			Modules = (FModule*) Malloc->Realloc(Modules, sizeof(FModule)* ModulesCapacity);
		}
		Modules[ModulesNum++] = Module;
		Algo::Sort(TArrayView<FModule>(Modules, ModulesNum), [](const FModule& A, const FModule& B) { return A.Id < B.Id; });
	}

#if BACKTRACE_DBGLVL >= 1
	NumFpTruncations += NumFpFuncs;
	TotalFunctions += NumFunctions;
#endif

#endif // 0
}

////////////////////////////////////////////////////////////////////////////////
void FBacktracer::RemoveModule(UPTRINT ModuleBase)
{
#if 0
	// When Windows' RequestExit() is called it hard-terminates all threads except
	// the main thread and then proceeds to unload the process' DLLs. This hard 
	// thread termination can result is dangling locked locks. Not an issue as
	// the rule is "do not do anything multithreaded in DLL load/unload". And here
	// we are, taking write locks during DLL unload which is, quite unsurprisingly,
	// deadlocking. In reality tracking Windows' DLL unloads doesn't tell us
	// anything due to how DLLs and processes' address spaces work. So we will...
#if defined PLATFORM_WINDOWS
	return;
#endif

	FScopeLock _(&Lock);

	uint32 ModuleId = AddressToId(ModuleBase);
	TArrayView<FModule> ModulesView(Modules, ModulesNum);
	int32 Index = Algo::LowerBound(ModulesView, ModuleId, FIdPredicate());
	if (Index >= ModulesNum)
	{
		return;
	}

	const FModule& Module = Modules[Index];
	if (Module.Id != ModuleId)
	{
		return;
	}

#if BACKTRACE_DBGLVL >= 1
	NumFpTruncations -= Module.NumFpTypes;
	TotalFunctions -= Module.NumFunctions;
#endif

	// no code should be executing at this point so we can safely free the
	// table knowing know one is looking at it.
	Malloc->Free(Module.Functions);

	for (SIZE_T i = Index; i < ModulesNum; i++)
	{
		Modules[i] = Modules[i + 1];
	}

	--ModulesNum;
#endif // 0
}

////////////////////////////////////////////////////////////////////////////////
const FBacktracer::FFunction* FBacktracer::LookupFunction(UPTRINT Address, FLookupState& State) const
{
#if 0
	// This function caches the previous module look up. The theory here is that
	// a series of return address in a backtrace often cluster around one module

	FIdPredicate IdPredicate;

	// Look up the module that Address belongs to.
	uint32 AddressId = AddressToId(Address);
	if ((AddressId - State.Module.Id) >= State.Module.IdSize)
	{
		TArrayView<FModule> ModulesView(Modules, ModulesNum);
		uint32 Index = Algo::UpperBound(ModulesView, AddressId, IdPredicate);
		if (Index == 0)
		{
			return nullptr;
		}

		State.Module = Modules[Index - 1];
	}

	// Check that the address is within the address space of the best-found module
	const FModule* Module = &(State.Module);
	if ((AddressId - Module->Id) >= Module->IdSize)
	{
		return nullptr;
	}

	// Now we've a module we have a table of functions and their stack sizes so
	// we can get the frame size for Address
	uint32 FuncId = uint32(Address - IdToAddress(Module->Id)); 
	TArrayView<FFunction> FuncsView(Module->Functions, Module->NumFunctions);
	uint32 Index = Algo::UpperBound(FuncsView, FuncId, IdPredicate);

	const FFunction* Function = Module->Functions + (Index - 1);
#if BACKTRACE_DBGLVL >= 2
	if ((FuncId - Function->Id) >= Function->Size)
	{
		PLATFORM_BREAK();
		return nullptr;
	}
#endif
	return Function;
#else
	return nullptr;
#endif // 0
}

////////////////////////////////////////////////////////////////////////////////
uint32 FBacktracer::GetBacktraceId(const uint32& Hash, uint32 NumFrames, void** Frames) const
{
	// Build the backtrace entry for submission
	FCallstackTracer::FBacktraceEntry BacktraceEntry;
	BacktraceEntry.Hash = Hash;
	BacktraceEntry.FrameCount = NumFrames;
	BacktraceEntry.Frames = Frames;
	// Add to queue to be processed. This might block until there is room in the
	// queue (i.e. the processing thread has caught up processing).
	return CallstackTracer->AddCallstack(BacktraceEntry);
}

////////////////////////////////////////////////////////////////////////////////
int32 FBacktracer::GetFrameSize(void* FunctionAddress) const
{
	std::scoped_lock<std::mutex> _(Lock);

	FLookupState LookupState = {};
	const FFunction* Function = LookupFunction(UPTRINT(FunctionAddress), LookupState);
	if (FunctionAddress == nullptr)
	{
		return -1;
	}

	if (Function->RspBias < 0)
	{
		return -1;
	}

	return Function->RspBias;
}
}
}