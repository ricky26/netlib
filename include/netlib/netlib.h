#pragma once

#ifdef _MSC_VER
#	ifdef NETLIB_EXPORTS
#		define NETLIB_API __declspec(dllexport)
#	else
#		define NETLIB_API __declspec(dllimport)
#	endif

	// Disable some stupid MSVC warnings
#	pragma warning(disable:4251 4996)
#else
#	ifdef NETLIB_EXPORTS
#		define NETLIB_API 
#	else
#		define NETLIB_API 
#	endif
#endif

#include <stdint.h>
#include <stddef.h>

namespace netlib
{
	template<typename T>
	inline void safe_delete(T *&_var)
	{
		T *val = _var;
		_var = NULL;
		delete val;
	}

	NETLIB_API bool init();
	NETLIB_API void exit(int _val=0);
	NETLIB_API int exit_value();
	NETLIB_API bool think();
	NETLIB_API int run_main_loop();
}