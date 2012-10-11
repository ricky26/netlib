#include "netlib/socket.h"
#include "netlib_linux.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <list>
#include <cstring>

namespace netlib
{
	//
	// socket_internal
	//
	
	struct socket_internal: public ref_counted
	{
		socket_internal(int _fd=-1)
		{
			acquire();
			fd = _fd;
		}

		~socket_internal()
		{
			if(fd != -1)
				::close(fd);
		}

		int fd;
		aio_struct aio;

		static inline socket_internal *get(void *_ptr)
		{
			return static_cast<socket_internal*>(_ptr);
		}
	};

	//
	// Helper functions
	//
	
	static bool getSocketParams(address_family _af, socket_type _sock, socket_protocol _prot,
		int &_oaf, int &_otype, int &_oprot)
	{
		switch(_af)
		{
		case af_inet:
			_oaf = AF_INET;
			break;

		default:
			return false;
		};

		switch(_sock)
		{
		case sock_stream:
			_otype = SOCK_STREAM;
			break;

		default:
			return false;
		};

		switch(_prot)
		{
		case prot_any:
			_oprot = 0;
			break;

		default:
			return false;
		};

		return true;
	}

	//
	// socket
	//

	socket::socket()
	{
		mInternal = nullptr;
	}
	
	socket::socket(address_family _af, socket_type _sock, socket_protocol _prot)
	{		
		int af, type, prot;
		if(!getSocketParams(_af, _sock, _prot, af, type, prot))
			mInternal = nullptr;
		else
		{
			socket_internal *si = new socket_internal();

			int fd = ::socket(af, type, prot);
			if(fd != -1)
				si->aio.make_nonblocking(fd);

			si->fd = fd;
			mInternal = si;
		}
	}

	socket::socket(int _sock)
	{
		socket_internal *si = new socket_internal(_sock);

		if(_sock != -1)
			si->aio.make_nonblocking(_sock);

		mInternal = si;
	}

	socket::socket(socket const& _sock)
	{
		socket_internal *si = socket_internal::get(_sock.mInternal);
		si->acquire();
		mInternal = si;
	}

	socket::socket(socket &&_s)
		: mInternal(_s.mInternal)
	{
		_s.mInternal = nullptr;
	}

	socket::~socket()
	{
		if(socket_internal *si = socket_internal::get(mInternal))
			si->release();
	}
	
	bool socket::valid() const
	{
		return socket_internal::get(mInternal)->fd != -1;
	}
	
	int socket::handle() const
	{
		return socket_internal::get(mInternal)->fd;
	}
	
	int socket::release()
	{
		socket_internal *si = socket_internal::get(mInternal);
		int ret = si->fd;
		si->fd = -1;
		return ret;
	}

	bool socket::connect(std::string const& _host, int _port)
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si->fd == -1)
			return false;

		hostent *he = gethostbyname(_host.c_str());
		if(!he)
			return false;

		si->aio.make_nonblocking(si->fd);

		// Connect

		sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = *((uint32_t*)he->h_addr_list[0]);
		std::memcpy((char*)he->h_addr, (char*)&addr.sin_addr.s_addr, he->h_length);
		addr.sin_port = htons(_port);

		si->aio.begin_out();
		int ret = ::connect(si->fd, (sockaddr*)&addr, sizeof(addr));
		if(ret < 0 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = ::connect(si->fd, (sockaddr*)&addr, sizeof(addr));
		}
		si->aio.end_out();

		if(ret < 0)
		{
			::close(si->fd);
			return false;
		}

		return true;
	}

	bool socket::listen(int _port, int _amt)
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si->fd == -1)
			return false;

		sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(_port);

		if(::bind(si->fd, (sockaddr*)&addr, sizeof(addr)) == -1)
		{
			std::cerr << "Failed to bind. (" << std::hex << errno << ')' << std::dec << std::endl;
			return false;
		}

		if(::listen(si->fd, _amt) == -1)
		{
			std::cerr << "Failed to listen." << std::endl;
			return false;
		}

		si->aio.make_nonblocking(si->fd);
		return true;
	}

	socket socket::accept()
	{
		socket_internal *si = socket_internal::get(mInternal);

		sockaddr_in addr;
		socklen_t len = sizeof(addr);

		si->aio.begin_in();
		int ret = ::accept(si->fd, (sockaddr*)&addr, &len);
		if(ret < 0 && errno == EAGAIN)
		{
			uthread::current()->suspend();
			ret = ::accept(si->fd, (sockaddr*)&addr, &len);
		}
		si->aio.end_in();

		if(ret < 0)
		{
			std::cerr << "Accept failed %d.\n" << errno << std::endl;
			close();
			return socket();
		}

		return socket(ret);
	}

	void socket::close()
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si != nullptr && si->fd != -1)
		{
			int fd = si->fd;
			si->fd = -1;
			::close(fd);
		}
	}

	size_t socket::read(void *_buffer, size_t _amt)
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si && si->fd != -1)
		{
			si->aio.begin_in();
			int ret = ::read(si->fd, _buffer, _amt);
			if(ret < 0 && errno == EAGAIN)
			{
				uthread::current()->suspend();
				ret = ::read(si->fd, _buffer, _amt);
			}
			si->aio.end_in();

			if(ret < 0)
			{
				close();
				return 0;
			}

			return ret;
		}

		return 0;
	}

	size_t socket::write(const void *_buffer, size_t _amt)
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si && si->fd != -1)
		{
			si->aio.begin_out();
			int ret = ::write(si->fd, _buffer, _amt);
			if(ret < 0 && errno == EAGAIN)
			{
				uthread::current()->suspend();
				ret = ::write(si->fd, _buffer, _amt);
			}
			si->aio.end_out();

			if(ret < 0)
			{
				close();
				return 0;
			}

			return ret;
		}

		return 0;
	}

	bool socket::init()
	{
		return true;
	}

	void socket::think()
	{
	}

	void socket::shutdown()
	{
	}
}
