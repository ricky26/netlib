#include "netlib/exception.h"

namespace netlib
{
	//
	// stack_trace
	//

	struct stack_trace_internal
	{
		
	};

	stack_trace::stack_trace()
		: internalized(nullptr)
	{
	}

	bool stack_trace::valid() const
	{
		return get() != nullptr;
	}

	bool stack_trace::capture(size_t _offset)
	{
		return false;
	}
}
