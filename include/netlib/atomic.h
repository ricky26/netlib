#include "netlib.h"
#include "compiler.h"

#pragma once

#ifdef NETLIB_COMPILER_MSVC

namespace netlib
{
	namespace atomic
	{
		/** Increment an integer atomically. */
		inline uint32_t increment(uint32_t *_ptr) { return _InterlockedIncrement((volatile long*)_ptr); }

		/** Decrement an integer atomically. */
		inline uint32_t decrement(uint32_t *_ptr) { return _InterlockedDecrement((volatile long*)_ptr); }

#ifdef NETLIB_X64
		/** Increment an integer atomically. */
		inline uint64_t increment(uint64_t *_ptr) { return _InterlockedIncrement64((volatile long long*)_ptr); }

		/** Decrement an integer atomically. */
		inline uint64_t decrement(uint64_t *_ptr) { return _InterlockedDecrement64((volatile long long*)_ptr); }
#endif
	}
}

#else // GCC

namespace netlib
{
	namespace atomic
	{
		/** Increment an integer atomically. */
		inline uint32_t increment(uint32_t *_ptr) { return __sync_add_and_fetch(_ptr, 1); }

		/** Decrement an integer atomically. */
		inline uint32_t decrement(uint32_t *_ptr) { return __sync_sub_and_fetch(_ptr, 1); }

		/** Increment an integer atomically. */
		inline uint64_t increment(uint64_t *_ptr) { return __sync_add_and_fetch(_ptr, 1); }

		/** Decrement an integer atomically. */
		inline uint64_t decrement(uint64_t *_ptr) { return __sync_sub_and_fetch(_ptr, 1); }
	}
}

#endif
