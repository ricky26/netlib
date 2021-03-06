#include "netlib/pipe.h"
#include "netlib/socket.h"
#include "netlib_linux.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

namespace netlib
{
	//
	// pipe_internal
	//
	
	struct pipe_internal: public ref_counted
	{
		int fd;
		aio_struct aio;

		pipe_internal(int _fd=-1)
		{
			fd = _fd;
		}

		~pipe_internal()
		{
			if(fd != -1)
				::close(fd);
		}

		static inline pipe_internal *get(void *_ptr)
		{
			return static_cast<pipe_internal*>(_ptr);
		}
	};

	//
	// pipe
	//

	pipe::pipe()
	{
		mInternal = nullptr;
	}

	pipe::pipe(int _handle)
	{
		pipe_internal *pi = new pipe_internal(_handle);

		if(_handle != -1)
			pi->aio.make_nonblocking(_handle);

		mInternal = pi;
	}

	pipe::pipe(pipe const& _p)
	{
		pipe_internal *pi = pipe_internal::get(_p.mInternal);
		pi->acquire();
		mInternal = pi;
	}

	pipe::pipe(pipe &&_p)
	{
		pipe_internal *pi = pipe_internal::get(_p.mInternal);
		_p.mInternal = nullptr;
		mInternal = pi;
	}

	pipe::~pipe()
	{
		if(pipe_internal *pi = pipe_internal::get(mInternal))
			pi->release();
	}

	bool pipe::valid() const
	{
		return pipe_internal::get(mInternal)->fd != -1;
	}

	int pipe::handle() const
	{
		return pipe_internal::get(mInternal)->fd;
	}

	int pipe::release()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);

		int hdl = pi->fd;
		pi->fd = -1;

		return hdl;
	}

	bool pipe::open(std::string const& _pipe)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd != -1)
			return false;

		int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
		if(fd == -1)
			return false;

		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		std::strcpy(addr.sun_path, _pipe.c_str());

		pi->aio.make_nonblocking(fd);

		int len = sizeof(addr.sun_family) + _pipe.length();

		pi->aio.begin_out();
		int ret = connect(fd, (sockaddr*)&addr, len);
		if(ret == -1 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = connect(fd, (sockaddr*)&addr, len);
		}
		pi->aio.end_out();

		if(ret < 0)
		{
			::close(fd);
			return false;
		}

		pi->fd = fd;
		return true;
	}

	bool pipe::create(std::string const& _pipe)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd != -1)
			return false;

		int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
		if(fd == -1)
			return false;

		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		std::strcpy(addr.sun_path, _pipe.c_str());

		int len = sizeof(addr.sun_family) + _pipe.length();
		int ret = bind(fd, (sockaddr*)&addr, len);
		if(ret < 0)
		{
			::close(fd);
			return false;
		}

		ret = ::listen(fd, 5);
		if(ret < 0)
		{
			::close(fd);
			return false;
		}

		pi->aio.make_nonblocking(fd);
		pi->fd = fd;
		return true;
	}

	pipe pipe::accept()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd == -1)
			return pipe();

		sockaddr_un addr;
		socklen_t len = sizeof(addr);

		pi->aio.begin_in();
		int ret = ::accept(pi->fd, (sockaddr*)&addr, &len);
		if(ret < 0 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = ::accept(pi->fd, (sockaddr*)&addr, &len);
		}
		pi->aio.end_in();

		if(ret == -1)
		{
			close();
			return pipe();
		}

		return std::move(pipe(ret));
	}

	void pipe::close()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd == -1)
			return;

		::close(pi->fd);
		pi->fd = -1;
	}

	size_t pipe::read(void *_buffer, size_t _amt)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd == -1)
			return 0;

		pi->aio.begin_in();
		int ret = ::read(pi->fd, _buffer, _amt);
		if(ret < 0 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = ::read(pi->fd, _buffer, _amt);
		}
		pi->aio.end_in();

		if(ret < 0)
		{
			close();
			return 0;
		}

		return ret;
	}

	size_t pipe::write(const void *_buffer, size_t _amt)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi->fd == -1)
			return 0;

		pi->aio.begin_out();
		int ret = ::write(pi->fd, _buffer, _amt);
		if(ret < 0 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = ::write(pi->fd, _buffer, _amt);
		}
		pi->aio.end_out();

		if(ret < 0)
		{
			close();
			return 0;
		}

		return ret;
	}
		
	socket pipe::read()
	{
		return socket(); // TODO: Write this.
	}

	bool pipe::write(socket &_sock)
	{
		return false; // TODO: Write zis.
	}

	bool pipe::init()
	{
		return true;
	}

	void pipe::think()
	{
	}

	void pipe::shutdown()
	{
	}
}
