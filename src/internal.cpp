#include "netlib/internal.h"

namespace netlib
{
	//
	// internalized
	//

	internalized::internalized(void *_internal)
		: mInternal(_internal)
	{
	}

	internalized::internalized(internalized const& _b)
	{
		internal *i = _b.get<internal>();
		i->acquire();
		mInternal = i;
	}
	
	internalized::~internalized()
	{
		internal *i = get<internal>();
		i->release();
	}

	internalized &internalized::operator =(internalized const& _b)
	{
		internal *oi = get<internal>();
		internal *i = _b.get<internal>();
		i->acquire();
		oi->release();
		mInternal = i;
		return *this;
	}
}