// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <stdint.h>

#if !defined(UE_TRACE_ENABLED)
#	define UE_TRACE_ENABLED (UNITY_EDITOR || UNITY_DEVELOPER_BUILD) && !UNITY_EXTERNAL_TOOL
#endif

// Concatenates two preprocessor tokens, performing macro expansion on them first
#define PREPROCESSOR_JOIN(x, y) PREPROCESSOR_JOIN_INNER(x, y)
#define PREPROCESSOR_JOIN_INNER(x, y) x##y

#define PREPROCESSOR_TO_STRING(x) PREPROCESSOR_TO_STRING_INNER(x)
#define PREPROCESSOR_TO_STRING_INNER(x) #x

#ifdef _MSC_VER
#define FORCEINLINE __forceinline									/* Force code to be inline */
#define FORCENOINLINE __declspec(noinline)							/* Force code to NOT be inline */
#else
#define FORCENOINLINE __attribute__((noinline))
#define FORCEINLINE inline __attribute__ ((always_inline))
#endif

/** Branch prediction hints */
#ifndef UT_LIKELY						/* Hints compiler that expression is likely to be true, much softer than UE_ASSUME - allows (penalized by worse performance) expression to be false */
#if ( defined(__clang__) || defined(__GNUC__) ) && (PLATFORM_UNIX)	// effect of these on non-Linux platform has not been analyzed as of 2016-03-21
#define UT_LIKELY(x)			__builtin_expect(!!(x), 1)
#else
    // the additional "!!" is added to silence "warning: equality comparison with exteraneous parenthese" messages on android
#define UT_LIKELY(x)			(!!(x))
#endif
#endif

#ifndef UT_UNLIKELY					/* Hints compiler that expression is unlikely to be true, allows (penalized by worse performance) expression to be true */
#if ( defined(__clang__) || defined(__GNUC__) ) && (PLATFORM_UNIX)	// effect of these on non-Linux platform has not been analyzed as of 2016-03-21
#define UT_UNLIKELY(x)			__builtin_expect(!!(x), 0)
#else
#define UT_UNLIKELY(x)			(x)
#endif
#endif

//#define UE_TRACE_API  
#define TRACELOG_API   

namespace UE
{
    namespace Trace
    {
        typedef char ANSICHAR;
        typedef wchar_t TCHAR, WIDECHAR;
        typedef uint64_t uint64;
        typedef int64_t int64;
        typedef uint32_t uint32;
        typedef int32_t int32;
        typedef uint16_t uint16;
        typedef int16_t int16;
        typedef uint8_t uint8;
        typedef int8_t int8;
        typedef size_t SIZE_T;

        template <int>
        struct PtrTTraits
        {
            typedef int POINTER_TYPE;
        };

        template <>
        struct PtrTTraits<4>
        {
            typedef uint32 POINTER_TYPE;
            typedef int32 IPOINTER_TYPE;
        };

        template <>
        struct PtrTTraits<8>
        {
            typedef uint64 POINTER_TYPE;
            typedef int64 IPOINTER_TYPE;
        };

        typedef typename PtrTTraits<sizeof(void*)>::POINTER_TYPE UPTRINT;
        typedef typename PtrTTraits<sizeof(void*)>::IPOINTER_TYPE PTRINT;
    }
}
