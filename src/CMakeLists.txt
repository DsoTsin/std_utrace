
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/
)

set (TRACELOG_SRCS
    Runtime/TraceLog/Private/Trace/BlockPool.cpp
    Runtime/TraceLog/Private/Trace/Channel.cpp
    Runtime/TraceLog/Private/Trace/Codec.cpp
    Runtime/TraceLog/Private/Trace/Control.cpp
    Runtime/TraceLog/Private/Trace/EventNode.cpp
    Runtime/TraceLog/Private/Trace/Field.cpp
    Runtime/TraceLog/Private/Trace/Tail.cpp
    Runtime/TraceLog/Private/Trace/TlsBuffer.cpp
    Runtime/TraceLog/Private/Trace/Trace.cpp
    Runtime/TraceLog/Private/Trace/Writer.cpp
    Runtime/TraceLog/Private/Trace/Important/Cache.cpp
    Runtime/TraceLog/Private/Trace/Important/SharedBuffer.cpp
)

if (WIN32)
    list(APPEND TRACELOG_SRCS Runtime/TraceLog/Private/Trace/Detail/Windows/WindowsTrace.cpp)
endif (WIN32)

source_group(TraceLog FILES ${TRACELOG_SRCS})

set (PROFILER_SRCS
    Runtime/Profiler/TraceAuxiliary.cpp
    Runtime/Profiler/Prefix.cpp
    Runtime/Profiler/GrowOnlyLockFreeHash.h
    Runtime/Profiler/Backtracer.h
    Runtime/Profiler/Backtracer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/utrace.h
)

if (ANDROID)
    list(APPEND XHOOK_SRCS 
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_core.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_elf.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_jni.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_log.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_util.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_version.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xhook.c
    )
    source_group(xhook FILES ${XHOOK_SRCS})
    list(APPEND PROFILER_SRCS Runtime/TraceLog/Private/Trace/Detail/Android/AndroidTrace.cpp)
endif (ANDROID)

source_group(Profiler FILES ${PROFILER_SRCS})

add_library(unreal_trace SHARED ${PROFILER_SRCS})

if (ANDROID)
    target_sources(unreal_trace PRIVATE ${XHOOK_SRCS})
    target_link_libraries(unreal_trace PUBLIC log)
endif (ANDROID)

target_compile_definitions(unreal_trace PRIVATE BUILD_UTRACE_LIB=1)