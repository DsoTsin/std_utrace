#include "Prefix.h"

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
	printf("printf test\n");
	UTRACE_AUTO("Unknown2");
	fflush(stdout);
	return 0;
}