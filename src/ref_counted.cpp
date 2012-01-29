#include "netlib/ref_counted.h"

namespace netlib
{
	ref_counted::ref_counted(): mRefCount(0), mWeak(nullptr)
	{
	}

	ref_counted::~ref_counted()
	{
		if(mWeak)
		{
			mWeak->mTarget = nullptr;
			mWeak->release();
			mWeak = nullptr;
		}
	}

	void ref_counted::destroy()
	{
		delete this;
	}
}
