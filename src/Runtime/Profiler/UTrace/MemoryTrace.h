#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"
#include "Runtime/TraceLog/Public/Trace/Detail/LogScope.h"

#ifdef __cplusplus
extern "C" {
#endif
    EXPORT_COREMODULE void utrace_mem_init();
    /**
     * Trace an allocation event.
     * @param Address Address of allocation
     * @param Size Size of allocation
     * @param Alignment Alignment of the allocation
     * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
     */
    EXPORT_COREMODULE void utrace_mem_alloc(UInt64 Address, UInt64 Size, UInt32 Alignment);

    /**
     * Trace a free event.
     * @param Address Address of the allocation being freed
     * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
     */
    EXPORT_COREMODULE void utrace_mem_free(UInt64 Address);

    /**
     * Trace a free related to a reallocation event.
     * @param Address Address of the allocation being freed
     * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
     */
    EXPORT_COREMODULE void utrace_mem_realloc_free(UInt64 Address);

    /** Trace an allocation related to a reallocation event.
     * @param Address Address of allocation
     * @param NewSize Size of allocation
     * @param Alignment Alignment of the allocation
     * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
     */
    EXPORT_COREMODULE void utrace_mem_realloc_alloc(UInt64 Address, UInt64 NewSize, UInt32 Alignment);

    EXPORT_COREMODULE SInt32 utrace_mem_announce_tag(SInt32 Tag, SInt32 ParentTag, const char* Display);
    EXPORT_COREMODULE SInt32 utrace_mem_name_tag(const char* TagName);
    EXPORT_COREMODULE SInt32 utrace_mem_get_active_tag();
#ifdef __cplusplus
}
#if UE_TRACE_ENABLED
class FMemScope
{
public:
    EXPORT_COREMODULE FMemScope(SInt32 InTag, bool bShouldActivate = true);
    //EXPORT_COREMODULE FMemScope(ELLMTag InTag, bool bShouldActivate = true);
    //EXPORT_COREMODULE FMemScope(const class FName& InName, bool bShouldActivate = true);
    //EXPORT_COREMODULE FMemScope(const UE::LLMPrivate::FTagData* TagData, bool bShouldActivate = true);
    EXPORT_COREMODULE ~FMemScope();
private:
    void ActivateScope(SInt32 InTag);
    UE::Trace::Private::FScopedLogScope Inner;
    SInt32 PrevTag;
};

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

#define UE_MEMSCOPE(InTag)				FMemScope PREPROCESSOR_JOIN(MemScope,__LINE__)(InTag);
#define UE_MEMSCOPE_PTR(InPtr)			FMemScopePtr PREPROCESSOR_JOIN(MemPtrScope,__LINE__)((uint64)InPtr);
#define UE_MEMSCOPE_DEFAULT(InTag)		FDefaultMemScope PREPROCESSOR_JOIN(MemScope,__LINE__)(InTag);

#else

#define UE_MEMSCOPE(...)
#define UE_MEMSCOPE_PTR(...)
#define UE_MEMSCOPE_DEFAULT(...)

#endif // UE_TRACE_ENABLED

#endif // __cplusplus
