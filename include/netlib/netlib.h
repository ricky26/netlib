#include "compiler.h"

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <functional>

namespace netlib
{
	template<typename T>
	inline void safe_delete(T *&_var)
	{
		T *val = _var;
		_var = nullptr;
		delete val;
	}

	NETLIB_API bool init();
	NETLIB_API void shutdown();

	NETLIB_API void exit(int _val=0);
	NETLIB_API int exit_value();
	NETLIB_API bool think();
	NETLIB_API bool running();
	NETLIB_API int run_main_loop();
	NETLIB_API int run_main(std::function<void()> const&);

	NETLIB_API void be_nice();
	NETLIB_API void sleep(int _ms);

	// Returns time from an aribtrary point in microseconds.
	NETLIB_API uint64_t time();
}