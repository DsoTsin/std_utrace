#pragma once
#include "Runtime/TraceLog/Public/Trace/Config.h"

#if UE_TRACE_ENABLED
enum ETraceCounterType
{
    TraceCounterType_Int,
    TraceCounterType_Float,
};

enum ETraceCounterDisplayHint
{
    TraceCounterDisplayHint_None,
    TraceCounterDisplayHint_Memory,
};

#define __TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, CounterType, CounterDisplayHint) \
	static FCountersTrace::CounterType PREPROCESSOR_JOIN(__TraceCounter, __LINE__)(CounterDisplayName, CounterDisplayHint);

#define TRACE_INT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, TraceCounterDisplayHint_None) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_FLOAT_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterFloat, TraceCounterDisplayHint_None) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_MEMORY_VALUE(CounterDisplayName, Value) \
	__TRACE_DECLARE_INLINE_COUNTER(CounterDisplayName, FCounterInt, TraceCounterDisplayHint_Memory) \
	PREPROCESSOR_JOIN(__TraceCounter, __LINE__).Set(Value);

#define TRACE_DECLARE_INT_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_None);

#define TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName) \
	extern FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName);

#define TRACE_DECLARE_FLOAT_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterFloat PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_None);

#define TRACE_DECLARE_FLOAT_COUNTER_EXTERN(CounterName) \
	extern FCountersTrace::FCounterFloat PREPROCESSOR_JOIN(__GTraceCounter, CounterName);

#define TRACE_DECLARE_MEMORY_COUNTER(CounterName, CounterDisplayName) \
	FCountersTrace::FCounterInt PREPROCESSOR_JOIN(__GTraceCounter, CounterName)(CounterDisplayName, TraceCounterDisplayHint_Memory);

#define TRACE_DECLARE_MEMORY_COUNTER_EXTERN(CounterName) \
	TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName)

#define TRACE_COUNTER_SET(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Set(Value);

#define TRACE_COUNTER_ADD(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Add(Value);

#define TRACE_COUNTER_SUBTRACT(CounterName, Value) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Subtract(Value);

#define TRACE_COUNTER_INCREMENT(CounterName) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Increment();

#define TRACE_COUNTER_DECREMENT(CounterName) \
	PREPROCESSOR_JOIN(__GTraceCounter, CounterName).Decrement();

struct FCountersTrace
{
    static uint16_t OutputInitCounter(const char* CounterName, ETraceCounterType CounterType, ETraceCounterDisplayHint CounterDisplayHint);
    static void OutputSetValue(uint16_t CounterId, int64_t Value);
    static void OutputSetValue(uint16_t CounterId, double Value);

    template<typename ValueType, ETraceCounterType CounterType>
    class TCounter
    {
    public:
        TCounter(const char* InCounterName, ETraceCounterDisplayHint InCounterDisplayHint)
            : Value(0)
            , CounterId(0)
            , CounterName(InCounterName)
            , CounterDisplayHint(InCounterDisplayHint)
        {
            CounterId = OutputInitCounter(InCounterName, CounterType, CounterDisplayHint);
        }

        void Set(ValueType InValue)
        {
            if (Value != InValue)
            {
                Value = InValue;
                OutputSetValue(CounterId, Value);
            }
        }

        void Add(ValueType InValue)
        {
            if (InValue != 0)
            {
                Value += InValue;
                OutputSetValue(CounterId, Value);
            }
        }

        void Subtract(ValueType InValue)
        {
            if (InValue != 0)
            {
                Value -= InValue;
                OutputSetValue(CounterId, Value);
            }
        }

        void Increment()
        {
            ++Value;
            OutputSetValue(CounterId, Value);
        }

        void Decrement()
        {
            --Value;
            OutputSetValue(CounterId, Value);
        }

    private:
        ValueType Value;
        uint16_t CounterId;
        const char* CounterName;
        ETraceCounterDisplayHint CounterDisplayHint;

        bool CheckCounterId()
        {
            CounterId = OutputInitCounter(CounterName, CounterType, CounterDisplayHint);
            return !!CounterId;
        }
    };

    using FCounterInt = TCounter<int64_t, TraceCounterType_Int>;
    using FCounterFloat = TCounter<double, TraceCounterType_Float>;
};

#else
#define TRACE_INT_VALUE(CounterDisplayName, Value)
#define TRACE_FLOAT_VALUE(CounterDisplayName, Value)
#define TRACE_MEMORY_VALUE(CounterDisplayName, Value)
#define TRACE_DECLARE_INT_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_INT_COUNTER_EXTERN(CounterName)
#define TRACE_DECLARE_FLOAT_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_FLOAT_COUNTER_EXTERN(CounterName)
#define TRACE_DECLARE_MEMORY_COUNTER(CounterName, CounterDisplayName)
#define TRACE_DECLARE_MEMORY_COUNTER_EXTERN(CounterName)
#define TRACE_COUNTER_SET(CounterName, Value)
#define TRACE_COUNTER_ADD(CounterName, Value)
#define TRACE_COUNTER_SUBTRACT(CounterName, Value)
#define TRACE_COUNTER_INCREMENT(CounterName)
#define TRACE_COUNTER_DECREMENT(CounterName)
#endif
