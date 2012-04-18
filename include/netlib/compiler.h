#pragma once

#ifdef _MSC_VER
#	include <intrin.h>
#	define NETLIB_COMPILER_MSVC
#	define NETLIB_API_EXPORT	__declspec(dllexport)
#	define NETLIB_API_IMPORT	__declspec(dllimport)
#	define NETLIB_INLINE		__forceinline
#	define NETLIB_FASTCALL		__fastcall
#	define NETLIB_THREAD		__declspec(thread)

#	ifdef _M_AMD64
#	define NETLIB_X64
#	elif _M_IX86
#	define NETLIB_X86
#	endif

	// Disable some stupid MSVC warnings
#	pragma warning(disable:4251 4996 4355)
	// Enable UTF-8 mode
#	pragma execution_character_set("utf-8")
#else
#	define NETLIB_COMPILER_GCC
#	define NETLIB_API_EXPORT 
#	define NETLIB_API_IMPORT 
#	define NETLIB_INLINE		inline
#	define NETLIB_FASTCALL		__msfastcall
#	define NETLIB_THREAD		__thread
#endif

#ifdef NETLIB_STATIC
#	define NETLIB_API
#else
#ifdef NETLIB_EXPORTS
#	define NETLIB_API			NETLIB_API_EXPORT
#else
#	define NETLIB_API			NETLIB_API_IMPORT
#endif
#endif

#define NETLIB_ATOMIC			NETLIB_API NETLIB_INLINE 