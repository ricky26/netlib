#include "netlib/ref_counted.h"

namespace netlib
{
	ref_counted::ref_counted(): mRefCount(0)
	{
	}

	ref_counted::~ref_counted()
	{
	}

	void ref_counted::destroy()
	{
		delete this;
	}
}
