#include "netlib/socket.h"
#include "netlib/ref_counted.h"
#include <stdint.h>
#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include "netlib_win32.h"

namespace netlib
{
	static LPFN_CONNECTEX ConnectEx = NULL;
	static LPFN_ACCEPTEX AcceptEx = NULL;

	//
	// socket_internal
	//
	struct socket_internal: public ref_counted
	{
		SOCKET handle;

		socket_internal(SOCKET _sock=INVALID_SOCKET)
			: handle(_sock)
		{
			acquire();

			if(handle != INVALID_SOCKET)
			{
				if(CreateIoCompletionPort((HANDLE)handle,
					gCompletionPort, 0, 0) != gCompletionPort)
				{
					closesocket(handle);
					handle = INVALID_SOCKET;
				}
			}
		}

		~socket_internal()
		{
			if(handle != INVALID_SOCKET)
				closesocket(handle);
		}
	
		static bool socket_params(address_family _af, socket_type _sock, socket_protocol _prot,
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

		static inline socket_internal *get(void *_ptr)
		{
			return static_cast<socket_internal*>(_ptr);
		}
	};

	//
	// socket
	//

	socket::socket()
	{
		mInternal = new socket_internal();
	}
	
	socket::socket(address_family _af, socket_type _sock, socket_protocol _prot)
	{		
		mInternal = new socket_internal();
		create(_af, _sock, _prot);
	}

	socket::socket(int _sock)
	{
		mInternal = new socket_internal((SOCKET)_sock);
	}
	
	socket::socket(socket const& _sock)
	{
		socket_internal *si = socket_internal::get(_sock.mInternal);
		si->acquire();
		mInternal = si;
	}

	socket::~socket()
	{
		if(socket_internal *si = socket_internal::get(mInternal))
			si->release();
	}

	bool socket::create(address_family _af, socket_type _sock,
			socket_protocol _prot)
	{
		if(valid())
			return false;

		socket_internal *si = socket_internal::get(mInternal);
		
		int af, type, prot;
		SOCKET sock;
		if(!socket_internal::socket_params(_af, _sock, _prot, af, type, prot))
			sock = INVALID_SOCKET;
		else
			sock = WSASocket(af, type, prot, NULL, 0, WSA_FLAG_OVERLAPPED);

		if(sock != INVALID_SOCKET)
		{
			if(CreateIoCompletionPort((HANDLE)sock,
				gCompletionPort, 0, 0) != gCompletionPort)
			{
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
		}
		si->handle = sock;

		return true;
	}
	
	bool socket::valid() const
	{
		return mInternal
			&& socket_internal::get(mInternal)->handle != INVALID_SOCKET;
	}
	
	int socket::handle() const
	{
		socket_internal *si = socket_internal::get(mInternal);
		return (int)si->handle;
	}
	
	int socket::release()
	{
		socket_internal *si = socket_internal::get(mInternal);
		int ret = (int)si->handle;
		si->handle = INVALID_SOCKET;
		return ret;
	}
	
	bool socket::connect(std::string const& _host, int _port)
	{
		if(!valid())
			return false;

		socket_internal *si = socket_internal::get(mInternal);

		hostent *he = gethostbyname(_host.c_str());
		if(!he)
			return false;

		// Find ConnectEx pointer!

		if(!ConnectEx)
		{
			GUID gufn = WSAID_CONNECTEX;
			DWORD dwBytes;

			if(WSAIoctl(si->handle,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&gufn, sizeof(gufn), &ConnectEx, sizeof(ConnectEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR)
				return false;
		}

		// Bind Source Socket

		sockaddr_in src_addr;
		memset(&src_addr, 0, sizeof(src_addr));
		src_addr.sin_family = AF_INET;
		src_addr.sin_addr.s_addr = INADDR_ANY;
		src_addr.sin_port = htons(0);

		if(bind(si->handle,
			(sockaddr*)&src_addr, sizeof(src_addr)) != 0)
			return false;
		
		// Connect

		iocp_async_state state;
		state.thread = uthread::current();

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = *((uint32_t*)he->h_addr_list[0]);
		memcpy((char*)he->h_addr, (char*)&addr.sin_addr.s_addr, he->h_length);
		addr.sin_port = htons(_port);

		if(ConnectEx(si->handle, (sockaddr*)&addr,
			sizeof(addr), NULL, 0, NULL,
			&state.overlapped) == FALSE)
		{
			if(WSAGetLastError() == WSA_IO_PENDING)
			{
				uthread::suspend();
				if(state.error != 0)
					return false;
			}
			else
			{
				std::cerr << "ERR: " << WSAGetLastError() << std::endl;
				return false;
			}
		}

		return true;
	}

	bool socket::listen(int _port, int _amt)
	{
		if(!valid())
			return false;

		socket_internal *si = socket_internal::get(mInternal);

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(_port);

		if(::bind(si->handle, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
			return false;
		}

		if(::listen(si->handle, _amt) == SOCKET_ERROR)
			return false;

		if(!AcceptEx)
		{
			GUID gufn = WSAID_ACCEPTEX;
			DWORD dwBytes;

			if(WSAIoctl(si->handle,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&gufn, sizeof(gufn), &AcceptEx, sizeof(AcceptEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR)
				return false;
		}

		return true;
	}

	socket socket::accept()
	{
		if(!valid())
			return socket();

		socket_internal *si = socket_internal::get(mInternal);

		socket ret(af_inet);

		iocp_async_state state;
		state.thread = uthread::current();
		
		char buf[sizeof(sockaddr_in)*2 + 32];
		
		if(AcceptEx(si->handle, (SOCKET)ret.handle(), buf,
			sizeof(buf) - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			&state.amount, &state.overlapped) == FALSE)
		{
			if(WSAGetLastError() == WSA_IO_PENDING)
			{
				state.thread = uthread::current();
				uthread::suspend();
				if(state.error != 0)
					ret.close();
			}
		}

		return ret;
	}

	void socket::close()
	{
		socket_internal *si = socket_internal::get(mInternal);

		if(si->handle != INVALID_SOCKET)
		{
			CloseHandle((HANDLE)si->handle);
			si->handle = INVALID_SOCKET;
		}
	}

	size_t socket::read(void *_buffer, size_t _amt)
	{
		socket_internal *si = socket_internal::get(mInternal);
		if(si->handle != INVALID_SOCKET)
		{
			iocp_async_state state;
			state.thread = uthread::current();

			int err;
			if(ReadFile((HANDLE)si->handle, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
				|| (err = WSAGetLastError()) == WSA_IO_PENDING)
			{
				uthread::suspend();
				err = state.error;
			}
			
			if(err)
			{
				std::cerr << "Recv Err: " << WSAGetLastError() << std::endl;
				close();
				return 0;
			}

			return state.amount;
		}

		return 0;
	}

	size_t socket::write(const void *_buffer, size_t _amt)
	{

		socket_internal *si = socket_internal::get(mInternal);
		if(si->handle != INVALID_SOCKET)
		{
			iocp_async_state state;
			state.thread = uthread::current();

			int err;
			if(WriteFile((HANDLE)si->handle, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
				|| (err = WSAGetLastError()) == WSA_IO_PENDING)
			{
				uthread::suspend();
				err = state.error;
			}

			if(err)
			{
				std::cerr << "WERR: " << GetLastError() << std::endl;
				close();
				return 0;
			}

			return state.amount;
		}

		return 0;
	}

	bool socket::init()
	{
		WSADATA wsa_data;
		if(WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0)
			return false;
		return true;
	}

	void socket::think()
	{
	}

	void socket::shutdown()
	{
		WSACleanup();
	}
}