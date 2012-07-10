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

	NETLIB_API void exit(int _val=0);

	NETLIB_API void idle(bool _can_block);

	NETLIB_API int run_main_loop();

	// Returns time from an aribtrary point in microseconds.
	NETLIB_API uint64_t time();
}