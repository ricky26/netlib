#include "netlib/module.h"
#include "netlib/internal.h"
#include "netlib/ref_counted.h"
#include <Windows.h>

namespace netlib
{
	//
	// module_internal
	//

	struct module_internal: public ref_counted
	{
		HMODULE handle;

		module_internal()
			: handle(NULL)
		{
			acquire();
		}
		
		~module_internal()
		{
			if(handle)
				FreeLibrary(handle);
		}

		static inline module_internal *get(void *_ptr)
		{
			return internal::get<module_internal>(_ptr);
		}
	};

	//
	// module
	//

	module::module()
	{
		internal::create<module_internal>(mInternal);
	}
	
	module::module(void *_h)
	{
		module_internal *mi = internal::create<module_internal>(mInternal);
		mi->handle = (HMODULE)_h;
	}

	module::module(module const& _mod)
	{
		module_internal *mi = module_internal::get(_mod.mInternal);
		mi->acquire();
		mInternal = mi;
	}

	module::module(std::string const& _nm)
	{
		internal::create<module_internal>(mInternal);
		open(_nm);
	}

	module::~module()
	{
		module_internal *mi = module_internal::get(mInternal);
		mi->release();
	}

	void *module::handle() const
	{
		module_internal *mi = module_internal::get(mInternal);
		return (void*)mi->handle;
	}

	bool module::valid() const
	{
		module_internal *mi = module_internal::get(mInternal);
		return mi->handle != NULL;
	}

	bool module::open(std::string const& _name)
	{
		if(valid())
			return false;
		
		module_internal *mi = module_internal::get(mInternal);
		HMODULE hMod = LoadLibraryA(_name.c_str());
		if(!hMod)
			return false;

		mi->handle = hMod;
		return true;
	}

	void module::close()
	{
		module_internal *mi = module_internal::get(mInternal);
		internal::create<module_internal>(mInternal);
		mi->release();
	}

	void *module::symbol(std::string const& _nm) const
	{
		if(!valid())
			return NULL;

		module_internal *mi = module_internal::get(mInternal);
		return GetProcAddress(mi->handle, _nm.c_str());
	}

	module &module::operator =(module const& _s)
	{
		module_internal *omi = module_internal::get(mInternal);
		module_internal *mi = module_internal::get(_s.mInternal);
		mi->acquire();
		omi->release();
		mInternal = mi;
		return *this;
	}
}