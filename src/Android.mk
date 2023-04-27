LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    	  	:= unreal_trace
LOCAL_CPP_EXTENSION 	:= .cpp

LOCAL_SRC_FILES 	  	:= 	Runtime/Profiler/TraceAuxiliary.cpp \
    						Runtime/Profiler/Prefix.cpp	\
							Runtime/Profiler/Backtracer.cpp \
							Runtime/TraceLog/Private/Trace/Detail/Android/AndroidTrace.cpp \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_core.c \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_elf.c \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_log.c \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_util.c \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xh_version.c \
							Runtime/TraceLog/Private/Trace/Detail/Android/xhook/xhook.c \
							mimalloc/src/alloc.c \
							mimalloc/src/alloc-aligned.c \
							mimalloc/src/alloc-posix.c \
							mimalloc/src/arena.c \
							mimalloc/src/bitmap.c \
							mimalloc/src/heap.c \
							mimalloc/src/init.c \
							mimalloc/src/options.c \
							mimalloc/src/os.c \
							mimalloc/src/page.c \
							mimalloc/src/random.c \
							mimalloc/src/segment.c \
							mimalloc/src/segment-map.c \
							mimalloc/src/stats.c \
							mimalloc/src/prim/prim.c

LOCAL_C_INCLUDES 	  	:= 	$(LOCAL_PATH) \
							$(LOCAL_PATH)/mimalloc/include \
							$(LOCAL_PATH)\..\inc
LOCAL_CPP_INCLUDES 	  	:= 	$(LOCAL_PATH) \
							$(LOCAL_PATH)\..\inc

LOCAL_LDLIBS 		    := -llog
LOCAL_LDFLAGS   	  	+= -Wl,--exclude-libs,ALL -Wl,-Bsymbolic-functions

ifeq ($(NDK_DEBUG),1)
	APP_STRIP_MODE 		:= none
else
	LOCAL_CPPFLAGS 		:= -std=c++17 -fno-exceptions -fvisibility=hidden -O2 -DLZ4LIB_VISIBILITY= -DXHOOK_EXPORT= -DBUILD_UTRACE_LIB=1 -DMI_TLS_RECURSE_GUARD -DMI_TLS_PTHREAD
	LOCAL_CFLAGS 		:= -fno-exceptions -fvisibility=hidden -O2 -DLZ4LIB_VISIBILITY= -DXHOOK_EXPORT= -DBUILD_UTRACE_LIB=1 -DMI_TLS_RECURSE_GUARD -DMI_TLS_PTHREAD
endif

include $(BUILD_SHARED_LIBRARY)