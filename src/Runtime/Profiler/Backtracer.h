#pragma once

#include "Prefix.h"
#include "GrowOnlyLockFreeHash.h"

namespace UE
{
namespace Trace
{
class FCallstackTracer;

class FBacktracer
{
public:
							FBacktracer();
							~FBacktracer();
	static FBacktracer*		Get();
	void					AddModule(UPTRINT Base, const TCHAR* Name);
	void					RemoveModule(UPTRINT Base);
	uint32					GetBacktraceId(const uint32& Hash, uint32 NumFrames, void** Frames) const;
	int32					GetFrameSize(void* FunctionAddress) const;

private:
	struct FFunction
	{
		uint32				Id;
		int32				RspBias;
#if BACKTRACE_DBGLVL >= 2
		uint32				Size;
		const FUnwindInfo*	UnwindInfo;
#endif
	};

	struct FModule
	{
		uint32				Id;
		uint32				IdSize;
		uint32				NumFunctions;
#if BACKTRACE_DBGLVL >= 1
		uint16				NumFpTypes;
		//uint16			*padding*
#else
		//uint32			*padding*
#endif
		FFunction*			Functions;
	};

	struct FLookupState
	{
		FModule				Module;
	};

	struct FFunctionLookupSetEntry
	{
		// Bottom 48 bits are key (pointer), top 16 bits are data (RSP bias for function)
		std::atomic_uint64_t Data;

		inline uint64 GetKey() const { return Data.load(std::memory_order_relaxed) & 0xffffffffffffull; }
		inline int32 GetValue() const { return static_cast<int64>(Data.load(std::memory_order_relaxed)) >> 48; }
		inline bool IsEmpty() const { return Data.load(std::memory_order_relaxed) == 0; }
		inline void SetKeyValue(uint64 Key, int32 Value)
		{
			Data.store(Key | (static_cast<int64>(Value) << 48), std::memory_order_relaxed);
		}
		static inline uint32 KeyHash(uint64 Key)
		{
			// 64 bit pointer to 32 bit hash
			Key = (~Key) + (Key << 21);
			Key = Key ^ (Key >> 24);
			Key = Key * 265;
			Key = Key ^ (Key >> 14);
			Key = Key * 21;
			Key = Key ^ (Key >> 28);
			Key = Key + (Key << 31);
			return static_cast<uint32>(Key);
		}
		static void ClearEntries(FFunctionLookupSetEntry* Entries, int32 EntryCount)
		{
			memset(Entries, 0, EntryCount * sizeof(FFunctionLookupSetEntry));
		}
	};
	typedef TGrowOnlyLockFreeHash<FFunctionLookupSetEntry, uint64, int32> FFunctionLookupSet;

	const FFunction*		LookupFunction(UPTRINT Address, FLookupState& State) const;
	static FBacktracer*		Instance;
	mutable std::mutex		Lock;
	FModule*				Modules;
	int32					ModulesNum;
	int32					ModulesCapacity;
	FCallstackTracer*		CallstackTracer;
#if BACKTRACE_LOCK_FREE
	mutable FFunctionLookupSet	FunctionLookups;
	mutable bool				bReentranceCheck = false;
#endif
#if BACKTRACE_DBGLVL >= 1
	mutable uint32			NumFpTruncations = 0;
	mutable uint32			TotalFunctions = 0;
#endif
};

}
}
