#include "Prefix.h"

void* category = (void*)0x56893909f;

int main(int argc, const char* argv[])
{
	utrace_initialize_main_thread(
		MAKE_UTRACE_STR("Test"), 
		MAKE_UTRACE_STR("Develop"), 
		MAKE_UTRACE_STR("-trace -tracehost=127.0.0.1"),
		MAKE_UTRACE_STR("Test"),
		10,
		UTRACE_TARGET_TYPE_GAME,
		UTRACE_BUILD_CONFIGURATION_DEVELOPMENT,
		0
	);
	utrace_try_connect();
	UTRACE_AUTO("Unknown");
	
	utrace_log_decl_category(category, MAKE_UTRACE_STR("Log"), UTRACE_LOG_TYPE_ALL);

	int point = 0;
	utrace_log_decl_spec(&point, category, UTRACE_LOG_TYPE_DISPLAY, MAKE_UTRACE_STR(__FILE__), __LINE__, "%s");
	utrace::log_write(&point, "New health");

	auto id = utrace_current_callstack_id();
	printf("printf test\n");
	UTRACE_AUTO("Unknown2");
	
    static TRACE_DECLARE_INT_COUNTER(ut_BatchesCount,   "[Render] Batches Count");
    TRACE_COUNTER_SET(ut_BatchesCount,                  100);

	fflush(stdout);
	return 0;
}