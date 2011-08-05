#include "netlib.h"

#pragma once

namespace netlib
{
	namespace atomic
	{
		NETLIB_ATOMIC uint32_t NETLIB_FASTCALL increment(uint32_t *_ptr);
		NETLIB_ATOMIC uint32_t NETLIB_FASTCALL decrement(uint32_t *_ptr);
	}
}
