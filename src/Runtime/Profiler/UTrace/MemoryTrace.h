#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"
#include "Runtime/TraceLog/Public/Trace/Detail/LogScope.h"

#ifdef __cplusplus
#if UE_TRACE_ENABLED
class FMemScope
{
public:
    EXPORT_COREMODULE FMemScope(SInt32 InTag, bool bShouldActivate = true);
    //EXPORT_COREMODULE FMemScope(ELLMTag InTag, bool bShouldActivate = true);
    EXPORT_COREMODULE FMemScope(const utrace_string& InName, bool bShouldActivate = true);
    //EXPORT_COREMODULE FMemScope(const UE::LLMPrivate::FTagData* TagData, bool bShouldActivate = true);
    EXPORT_COREMODULE ~FMemScope();
private:
    void ActivateScope(SInt32 InTag);
    UE::Trace::Private::FScopedLogScope Inner;
    SInt32 PrevTag;
};

static_assert(sizeof(FMemScope) == 8, "");
static_assert(alignof(FMemScope) == 4, "");

template<typename TagType>
class FDefaultMemScope : public FMemScope
{
public:
    FDefaultMemScope(TagType InTag)
        : FMemScope(InTag, utrace_mem_get_active_tag() == 0)
    {
    }
};

class FMemScopePtr
{
public:
    EXPORT_COREMODULE FMemScopePtr(UInt64 InPtr);
    EXPORT_COREMODULE ~FMemScopePtr();
private:
    UE::Trace::Private::FScopedLogScope Inner;
};

static_assert(sizeof(FMemScopePtr) <= 4, "");
static_assert(alignof(FMemScopePtr) == 1, "");

#define UE_MEMSCOPE(InTag)				FMemScope PREPROCESSOR_JOIN(MemScope,__LINE__)(InTag);
#define UE_MEMSCOPE_PTR(InPtr)			FMemScopePtr PREPROCESSOR_JOIN(MemPtrScope,__LINE__)((uint64)InPtr);
#define UE_MEMSCOPE_DEFAULT(InTag)		FDefaultMemScope PREPROCESSOR_JOIN(MemScope,__LINE__)(InTag);

#else

#define UE_MEMSCOPE(...)
#define UE_MEMSCOPE_PTR(...)
#define UE_MEMSCOPE_DEFAULT(...)

#endif // UE_TRACE_ENABLED

#endif // __cplusplus
