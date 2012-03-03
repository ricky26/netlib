#include "netlib/shared_memory.h"
#include <Windows.h>

namespace netlib
{
	//
	// shmem_private
	//

	struct shmem_internal
	{
		HANDLE handle;
		size_t size;
		void *pointer;

		shmem_internal()
		{
			handle = INVALID_HANDLE_VALUE;
			size = 0;
			pointer = nullptr;
		}

		static shmem_internal *get(void *_p)
		{
			return static_cast<shmem_internal*>(_p);
		}
	};


	//
	// shared_memory
	//

	shared_memory::shared_memory()
	{
		mInternal = new shmem_internal();
	}

	shared_memory::shared_memory(std::string const& _name, size_t _sz)
	{
		mInternal = new shmem_internal();
		open(_name, _sz);
	}
	
	shared_memory::shared_memory(shared_memory &&_mem)
		: mInternal(_mem.mInternal)
	{
		_mem.mInternal = nullptr;
	}

	shared_memory::~shared_memory()
	{
		if(mInternal)
		{
			close();

			shmem_internal *si = shmem_internal::get(mInternal);
			mInternal = nullptr;

			delete si;
		}
	}

	bool shared_memory::valid() const
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		return si && si->handle != INVALID_HANDLE_VALUE;
	}

	size_t shared_memory::size() const
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(si)
			return si->size;

		return 0;
	}

	void shared_memory::close()
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle == INVALID_HANDLE_VALUE)
			return;

		UnmapViewOfFile(si->pointer);
		CloseHandle(si->handle);

		si->pointer = nullptr;
		si->size = 0;
		si->handle = INVALID_HANDLE_VALUE;
	}

	bool shared_memory::open(std::string const& _name, size_t _sz)
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle != INVALID_HANDLE_VALUE)
			return false;

		std::string name = "Global\\" + _name;
		HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, _name.c_str());
		if(hMap == INVALID_HANDLE_VALUE)
			return false;


		void *pPtr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, _sz);
		if(!pPtr)
		{
			CloseHandle(hMap);
			return false;
		}

		si->handle = hMap;
		si->pointer = pPtr;
		si->size = _sz;
		return true;
	}

	bool shared_memory::create(std::string const& _name, size_t _sz)
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle != INVALID_HANDLE_VALUE)
			return false;

		std::string name = "Global\\" + _name;
		HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			0, (DWORD)_sz, _name.c_str());
		if(hMap == INVALID_HANDLE_VALUE)
			return false;

		void *pPtr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, _sz);
		if(!pPtr)
		{
			CloseHandle(hMap);
			return false;
		}

		si->handle = hMap;
		si->pointer = pPtr;
		si->size = _sz;
		return true;
	}
	
	void *shared_memory::pointer() const
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle == INVALID_HANDLE_VALUE)
			return nullptr;

		return si->pointer;
	}
}
