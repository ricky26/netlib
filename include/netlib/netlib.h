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
	NETLIB_API void idle_slave(bool _can_block);

	NETLIB_API int run_main_loop();
	NETLIB_API void spawn_io_threads(int _count=1);

	// Returns time from an aribtrary point in microseconds.
	NETLIB_API uint64_t time();

	NETLIB_API void update();

	NETLIB_API void process_io(bool _can_block=false);
	NETLIB_API bool process_io_single(int _timeout=0);

	NETLIB_API void process_messages();
	NETLIB_API bool process_message();
}