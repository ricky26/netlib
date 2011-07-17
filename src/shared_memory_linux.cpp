#include "netlib/shared_memory.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace netlib
{
	//
	// shmem_private
	//

	struct shmem_internal
	{
		int handle;
		size_t size;
		void *pointer;

		shmem_internal()
		{
			handle = -1;
			size = 0;
			pointer = NULL;
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

	shared_memory::~shared_memory()
	{
		if(mInternal)
		{
			close();

			shmem_internal *si = shmem_internal::get(mInternal);
			mInternal = NULL;

			delete si;
		}
	}

	bool shared_memory::valid() const
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		return si && si->handle != -1;
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
		if(!si || si->handle == -1)
			return;

		munmap(si->pointer, si->size);
		::close(si->handle);

		si->pointer = NULL;
		si->size = 0;
		si->handle = -1;
	}

	bool shared_memory::open(std::string const& _name, size_t _sz)
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle != -1)
			return false;

		int fd = shm_open(_name.c_str(), O_RDWR, 0664);
		if(fd == -1)
			return false;

		void *ptr = mmap(NULL, _sz, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
		if(!ptr)
		{
			::close(fd);
			return false;
		}

		si->handle = fd;
		si->pointer = ptr;
		si->size = _sz;
		return true;
	}

	bool shared_memory::create(std::string const& _name, size_t _sz)
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle != -1)
			return false;

		int fd = shm_open(_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0664);
		if(fd == -1)
			return false;

		void *ptr = mmap(NULL, _sz, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
		if(!ptr)
		{
			::close(fd);
			return false;
		}

		si->handle = fd;
		si->pointer = ptr;
		si->size = _sz;
		return true;
	}
	
	void *shared_memory::pointer() const
	{
		shmem_internal *si = shmem_internal::get(mInternal);
		if(!si || si->handle == -1)
			return NULL;

		return si->pointer;
	}
}
