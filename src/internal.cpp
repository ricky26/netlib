#include "netlib/internal.h"

namespace netlib
{
	//
	// internalized
	//

	internalized::internalized(internal *_internal)
		: mInternal(_internal)
	{
	}

	internalized::internalized(internalized const& _b)
	{
		internal *i = _b.get<internal>();
		if(i)
			i->acquire();
		mInternal = i;
	}
	
	internalized::internalized(internalized &&_b)
		: mInternal(_b.mInternal)
	{
		_b.mInternal = nullptr;
	}
	
	internalized::~internalized()
	{
		internal *i = get<internal>();
		if(i)
			i->release();
	}

	internalized &internalized::operator =(internalized const& _b)
	{
		internal *oi = get<internal>();
		internal *i = _b.get<internal>();

		if(i)
			i->acquire();

		if(oi)
			oi->release();

		mInternal = i;
		return *this;
	}
	
	void internalized::set(internal *_ptr)
	{
		if(_ptr == mInternal)
			return;

		_ptr->acquire();

		if(mInternal)
			mInternal->release();

		mInternal = _ptr;
	}
}