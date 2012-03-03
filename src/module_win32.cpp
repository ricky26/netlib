#include "netlib/module.h"
#include "netlib/internal.h"
#include "netlib/ref_counted.h"
#include <memory>
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
			: handle(nullptr)
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
	
	module::module(module &&_mod)
	{
		module_internal *mi = module_internal::get(_mod.mInternal);
		mInternal = mi;
		_mod.mInternal = nullptr;
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
		return mi->handle != nullptr;
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
		module_internal *mi = module_internal::get(mInternal);
		return GetProcAddress(mi->handle, _nm.c_str());
	}
	
	file module::file() const
	{
		module_internal *mi = module_internal::get(mInternal);

		// This code is frankly stupid, because Microsoft
		// never figured anyone would need to get the length
		// of a module path! -- Ricky26

		wchar_t path[512];
		std::unique_ptr<wchar_t> ppath;
		size_t psize = sizeof(path);
		SetLastError(0);

		DWORD size = GetModuleFileNameW(mi->handle, path, sizeof(path));
		
		int err = GetLastError();
		while(err == ERROR_INSUFFICIENT_BUFFER)
		{
			psize *= 2;

			if(ppath.get() == nullptr)
				ppath.reset((wchar_t*)malloc(psize));
			else
				ppath.reset((wchar_t*)realloc(ppath.get(), psize));

			size = GetModuleFileNameW(mi->handle, ppath.get(), (DWORD)psize);
			err = GetLastError();
		}

		if(err)
			return netlib::file();

		wchar_t *ptr = path;
		if(ppath.get())
			ptr = ppath.get();

		HANDLE h = CreateFileW(ptr, 0, 0, NULL, OPEN_ALWAYS, 0, 0);
		if(!h)
			return netlib::file();

		return netlib::file((void*)h);
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