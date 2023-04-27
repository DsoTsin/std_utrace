
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
        #Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_jni.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_log.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_util.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_version.c
        Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xhook.c
    )
    source_group(xhook FILES ${XHOOK_SRCS})
    list(APPEND PROFILER_SRCS Runtime/TraceLog/Private/Trace/Detail/Android/AndroidTrace.cpp)
endif (ANDROID)

source_group(Profiler FILES ${PROFILER_SRCS})

set(mi_sources
    mimalloc/src/alloc.c
    mimalloc/src/alloc-aligned.c
    mimalloc/src/alloc-posix.c
    mimalloc/src/arena.c
    mimalloc/src/bitmap.c
    mimalloc/src/heap.c
    mimalloc/src/init.c
    mimalloc/src/options.c
    mimalloc/src/os.c
    mimalloc/src/page.c
    mimalloc/src/random.c
    mimalloc/src/segment.c
    mimalloc/src/segment-map.c
    mimalloc/src/stats.c
    mimalloc/src/prim/prim.c)

source_group(Mimalloc FILES ${mi_sources})

add_library(unreal_trace SHARED ${PROFILER_SRCS})

if (ANDROID)
    target_sources              (unreal_trace PRIVATE ${XHOOK_SRCS} ${mi_sources})
    target_link_libraries       (unreal_trace PUBLIC  log)
    target_link_options         (unreal_trace PRIVATE "-Wl,--exclude-libs,ALL" "-whyload")
    target_compile_options      (unreal_trace PRIVATE "-fvisibility=hidden")
    target_compile_definitions  (unreal_trace PRIVATE "LZ4LIB_VISIBILITY=" "XHOOK_EXPORT=")
endif (ANDROID)

if (WIN32)
    set_target_properties       (unreal_trace PROPERTIES 
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#    target_compile_options      (unreal_trace PUBLIC "/Zc:__cplusplus")
endif(WIN32)

target_compile_definitions(unreal_trace PRIVATE BUILD_UTRACE_LIB=1 MI_TLS_RECURSE_GUARD MI_TLS_PTHREAD)
target_include_directories(unreal_trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mimalloc/include)