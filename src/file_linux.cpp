#include "netlib/file.h"
#include "netlib_linux.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

namespace netlib
{
	//
	// file_internal
	//

	struct file_internal: public ref_counted
	{
		int handle;
		off_t position;
		aio_struct aio;

		file_internal(int _hdl=-1)
		{
			handle = _hdl;
			position = 0;
			acquire();
		}

		~file_internal()
		{
			if(handle != -1)
				::close(handle);
		}

		static inline file_internal *get(void *_ptr)
		{
			return static_cast<file_internal*>(_ptr);
		}
	};

	//
	// file
	//

	file::file()
	{
		mInternal = new file_internal();
	}

	file::file(file const& _f)
	{
		file_internal *fi = file_internal::get(_f.mInternal);
		fi->acquire();
		mInternal = fi;
	}

	file::file(file &&_f)
	{
		file_internal *fi = file_internal::get(_f.mInternal);
		mInternal = fi;
		_f.mInternal = nullptr;
	}

	file::file(std::string const& _path, int _mode)
	{
		mInternal = new file_internal();
		open(_path, _mode);
	}

	file::~file()
	{
		if(file_internal *fi = file_internal::get(mInternal))
			fi->release();
	}

	bool file::valid() const
	{
		return mInternal
			&& file_internal::get(mInternal)->handle != -1;
	}

	bool file::open(std::string const& _path, int _mode)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle != -1)
			return false;

		int flags = O_NONBLOCK;
		bool r = _mode & mode_read;
		bool w = _mode & mode_write;
		if(r && w)
			flags |= O_RDWR;
		else if(r)
			flags |= O_RDONLY;
		else if(w)
			flags |= O_WRONLY;

		int creat;
		switch(_mode & open_mask)
		{
		case mode_create:
			flags |= O_CREAT;
			break;

		case mode_append:
			flags |= O_APPEND;
			break;

		default:
			break;
		}

		int fd = ::open(_path.c_str(), flags, 0664);
		if(fd == -1)
			return false;

		fi->aio.make_nonblocking(fd);
		fi->handle = fd;
		fi->position = 0;
		return true;
	}

	void file::close()
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == -1)
			return;

		::close(fi->handle);
		fi->handle = -1;
		fi->position = 0;
	}

	uint64_t file::size()
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == -1)
			return 0;

		uint64_t pos = ::lseek(fi->handle, 0, SEEK_CUR);
		uint64_t ret = ::lseek(fi->handle, 0, SEEK_END);
		::lseek(fi->handle, pos, SEEK_SET);
		return ret;
	}
		
	size_t file::read(void *_buffer, size_t _amt)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(fi && fi->handle != -1)
		{
			fi->aio.begin_in();

			int ret = pread(fi->handle, _buffer, _amt, fi->position);
			if(ret == -1 && errno == EAGAIN)
			{
				uthread::current()->suspend();
				ret = pread(fi->handle, _buffer, _amt, fi->position);
			}

			fi->aio.end_in();
		
			if(ret == -1)
			{
				close();
				return 0;
			}

			return ret;
		}

		return 0;
	}
		
	size_t file::write(const void *_buffer, size_t _amt)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(fi && fi->handle != -1)
		{
			fi->aio.begin_out();
			int ret = pwrite(fi->handle, _buffer, _amt, fi->position);
			if(ret == -1 && errno == EAGAIN)
			{
				uthread::current()->suspend();
				ret = pwrite(fi->handle, _buffer, _amt, fi->position);
			}
			fi->aio.end_out();

			if(ret == -1)
			{
				close();
				return 0;
			}

			return ret;
		}

		return 0;
	}
	
	bool file::seek(seek_pos_t _pos, seek_t _mode)
	{
		file_internal *fi = file_internal::get(mInternal);
		if(!fi || fi->handle == -1)
			return false;

		int mode;
		switch(_mode)
		{
		case from_start:
			mode = SEEK_SET;
			break;
		case from_end:
			mode = SEEK_END;
			break;
		case relative:
			mode = SEEK_CUR;
			break;
		};

		fi->position = lseek(fi->handle, _pos, mode);
		return true;
	}

	void file::flush()
	{
		if(!valid())
			return;
	}

	bool file::init()
	{
		return true;
	}


	void file::think()
	{
	}

	void file::shutdown()
	{
	}
}
