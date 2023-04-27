#ifndef __utrace_h__
#define __utrace_h__

#ifdef BUILD_UTRACE_LIB
#ifdef _MSC_VER
#define UTRACE_API __declspec(dllexport)
#else
#define UTRACE_API __attribute__((visibility("default")))
#endif
#else
#define UTRACE_API   
#endif

#define MAKE_UTRACE_STR(x) { x, __builtin_strlen(x) }

#include <stdint.h>

typedef uint64_t	utrace_u64;
typedef uint32_t	utrace_u32;
typedef uint8_t		utrace_u8;

#ifdef __cplusplus
extern "C" {
#endif
	typedef enum utrace_target_type {
		UTRACE_TARGET_TYPE_UNKNOWN,
		UTRACE_TARGET_TYPE_GAME,
		UTRACE_TARGET_TYPE_SERVER,
		UTRACE_TARGET_TYPE_CLIENT,
		UTRACE_TARGET_TYPE_EDITOR,
		UTRACE_TARGET_TYPE_PROGRAM,
	} utrace_target_type;

	typedef enum utrace_build_configuration {
		UTRACE_BUILD_CONFIGURATION_UNKNOWN,
		UTRACE_BUILD_CONFIGURATION_DEBUG,
		UTRACE_BUILD_CONFIGURATION_DEBUG_GAME,
		UTRACE_BUILD_CONFIGURATION_DEVELOPMENT,
		UTRACE_BUILD_CONFIGURATION_SHIPPING,
		UTRACE_BUILD_CONFIGURATION_TEST,
	} utrace_build_configuration;

	typedef struct utrace_string {
		const char* str;
		utrace_u32 len;
	} utrace_string;

	UTRACE_API void utrace_initialize_main_thread(
		utrace_string appName,
		utrace_string buildVersion,
		utrace_string commandLine,
		utrace_string branchName,
		utrace_u32 changeList,
		utrace_target_type target_type,
		utrace_build_configuration build_configuration,
		utrace_u32 tailBytes
	);

	UTRACE_API void utrace_try_connect();
	UTRACE_API utrace_u64 utrace_get_timestamp();
	UTRACE_API int utrace_screenshot_enabled();

	UTRACE_API void utrace_write_screenshot(
		utrace_string shotName,
		utrace_u64 timestamp, 
		utrace_u32 width, utrace_u32 height, 
		unsigned char* imagePNGData, utrace_u32 imagePNGSize
	);

	UTRACE_API void utrace_register_thread(
		const char* name, 
		utrace_u32 sysId, 
		int sortHint
	);
	
	UTRACE_API void utrace_cpu_begin_name(const char* name, const char* file, int line);
	UTRACE_API void utrace_cpu_begin_name2(const char* name, int name_len, const char* file_name, int file_name_len, int line);
	UTRACE_API void utrace_cpu_end();

	UTRACE_API void utrace_file_begin_open(utrace_string path);
	UTRACE_API void utrace_file_fail_open(utrace_string path);
	UTRACE_API void utrace_file_begin_reopen(utrace_u64 oldHandle);
	UTRACE_API void utrace_file_end_reopen(utrace_u64 newHandle);
	UTRACE_API void utrace_file_end_open(utrace_u64 handle);
	UTRACE_API void utrace_file_begin_close(utrace_u64 handle);
	UTRACE_API void utrace_file_end_close(utrace_u64 handle);
	UTRACE_API void utrace_file_fail_close(utrace_u64 handle);
    UTRACE_API void utrace_file_begin_read(utrace_u64 readHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size);
    UTRACE_API void utrace_file_end_read(utrace_u64 readHandle, utrace_u64 sizeRead);
    UTRACE_API void utrace_file_begin_write(utrace_u64 writeHandle, utrace_u64 fileHandle, utrace_u64 offset, utrace_u64 size);
    UTRACE_API void utrace_file_end_write(utrace_u64 writeHandle, utrace_u64 sizeWritten);

    UTRACE_API void utrace_mem_init();
    UTRACE_API void utrace_mem_alloc(utrace_u64 address, utrace_u64 size, utrace_u32 alignment);
    UTRACE_API void utrace_mem_free(utrace_u64 address);
    UTRACE_API void utrace_mem_realloc_free(utrace_u64 address);
    UTRACE_API void utrace_mem_realloc_alloc(utrace_u64 address, utrace_u64 newSize, utrace_u32 alignment);
    UTRACE_API int	utrace_mem_announce_tag(int tag, int parentTag, const char* display);
    UTRACE_API int	utrace_mem_name_tag(const char* tagName);
    UTRACE_API int	utrace_mem_get_active_tag();
	
	UTRACE_API void	utrace_module_init(utrace_string fmt, unsigned char shift);
	UTRACE_API void	utrace_module_load(utrace_string name, utrace_u64 base, utrace_u32 size, const utrace_u8* imageIdData, utrace_u32 imageIdLen);
	UTRACE_API void	utrace_module_unload(utrace_u64 base);

	UTRACE_API utrace_u32 utrace_current_callstack_id();

#ifdef __cplusplus
}

namespace utrace {
struct Scope {
	Scope(const char* name, int nameLen, const char* filename, int filenameLen, int fileLine) {
		utrace_cpu_begin_name2(name, nameLen, filename, filenameLen, fileLine);
	}
	Scope(const char* name, const char* filename, int fileLine) {
		utrace_cpu_begin_name(name, filename, fileLine);
	}
	~Scope() {
		utrace_cpu_end();
	}
};
}

#define __PP_CONCAT_Y(A_, B_) A_##B_
#define __PP_CONCAT_X(A_, B_) __PP_CONCAT_Y(A_, B_)
#define UTRACE_AUTO(x) utrace::Scope __PP_CONCAT_X(__$utrace, __LINE__)((x), __builtin_strlen(x), __FILE__, __builtin_strlen(__FILE__), __LINE__)

#endif

#endif