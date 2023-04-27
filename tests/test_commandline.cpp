#include "Prefix.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, const char* argv[])
{
	const char* a0 = "";
	const char* a1 = "-trace";
	const char* a2 = "-trace=default,memory,perffeto";
	const char* a3 = "-trace -traceHost=127.0.0.1 ";
	// toughest
	const char* a4 = " -trace=default   -traceHost=\"baidu .com\" \"st op\" \"  ";
	core::ParseCommandline(a4, strlen(a4));
	core::ParseCommandline(a0, strlen(a0));
	core::ParseCommandline(a1, strlen(a1));
	core::ParseCommandline(a2, strlen(a2));
	core::ParseCommandline(a3, strlen(a3));

	Thread thread;
	thread.SetName("Test");
	thread.Run(
	[](void* data) -> void*
	{
#ifdef _WIN32
		SetThreadDescription(GetCurrentThread(), L"ThisIsMyThreadName!");
#endif
		printf("");
		return nullptr;
	}, nullptr, 0);
	thread.WaitForExit();
	return 0;
}