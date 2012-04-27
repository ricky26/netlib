#include "internal.h"

#pragma once

namespace netlib
{
	class NETLIB_API stack_trace: public internalized
	{
	public:
		stack_trace();

		bool valid() const;
		bool capture(size_t _offset=0);
	};
};
