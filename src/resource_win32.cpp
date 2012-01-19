#include "netlib/resource.h"
#include "netlib/ref_counted.h"
#include "netlib/internal.h"
#include <Windows.h>

namespace netlib
{
	struct resource_internal: public ref_counted
	{
		HRSRC resource;

		resource_internal(): resource(NULL)
		{
			acquire();
		}

		static inline resource_internal *get(void *_ptr)
		{
			return internal::get<resource_internal>(_ptr);
		}
	};
	
	resource::resource(module const& _m, std::string const& _name)
	{
		internal::create<resource_internal>(mInternal);
		open(_m, _name);
	}

	resource::resource()
	{
		internal::create<resource_internal>(mInternal);
	}

	resource::resource(resource const& _res)
	{
		resource_internal *ri = resource_internal::get(_res.mInternal);
		ri->acquire();
		mInternal = ri;
	}

	resource::~resource()
	{
		resource_internal *ri = resource_internal::get(mInternal);
		ri->release();
	}

	bool resource::valid() const
	{
		resource_internal *ri = resource_internal::get(mInternal);
		return ri->resource != NULL;
	}

	bool resource::open(module const& _m, std::string const& _nm)
	{
		resource_internal *ri = resource_internal::get(mInternal);
		if(ri->resource)
			return false;

		HRSRC rc = FindResourceA((HMODULE)_m.handle(), _nm.c_str(), "DATA");
		if(!rc)
			return false;

		ri->resource = rc;
		return true;
	}

	void resource::close()
	{
		resource_internal *ri = resource_internal::get(mInternal);
		if(!ri->resource)
			return;
		
		ri->resource = NULL;
	}

	size_t resource::read(void *_buffer, size_t _amt)
	{
		return 0;
	}

	size_t resource::write(const void *_buffer, size_t _amt)
	{
		return 0;
	}
}
