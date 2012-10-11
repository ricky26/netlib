#include "netlib/socket.h"
#include "netlib/ref_counted.h"
#include <stdint.h>
#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include "netlib_win32.h"

namespace netlib
{
	//
	// socket_internal
	//
	struct socket_internal: public ref_counted
	{
		SOCKET handle;
		LPFN_CONNECTEX connectEx;
		LPFN_ACCEPTEX acceptEx;

		socket_internal(SOCKET _sock=INVALID_SOCKET)
			: handle(_sock), connectEx(nullptr), acceptEx(nullptr)
		{
			acquire();
			add_to_completion();
		}

		void add_to_completion()
		{
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
		mInternal = nullptr;
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

		si->handle = sock;
		si->add_to_completion();
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
		socket_internal *si = socket_internal::get(mInternal);
		if(!si || si->handle == INVALID_SOCKET)
			return false;
		
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.s_addr = inet_addr(_host.c_str());
		if(addr.sin_addr.s_addr == INADDR_NONE)
		{
			hostent *he = gethostbyname(_host.c_str());
			if(!he)
				return false;

			memcpy((char*)&addr.sin_addr.s_addr, (char*)he->h_addr, he->h_length);
		}

		// Find ConnectEx pointer!

		if(!si->connectEx)
		{
			GUID gufn = WSAID_CONNECTEX;
			DWORD dwBytes;

			if(WSAIoctl(si->handle,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&gufn, sizeof(gufn), &si->connectEx, sizeof(si->connectEx),
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

		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);
		
		uthread::current()->suspend([&](){
			if(si->connectEx(si->handle, (sockaddr*)&addr,
				sizeof(addr), NULL, 0, NULL,
				&state.overlapped) == FALSE)
			{
				int error = WSAGetLastError();
				if(error != WSA_IO_PENDING)
				{
					std::cerr << "ERR: " << error << std::endl;
					state.error = error;
					state.thread->resume();
				}
			}
		});

		return state.error != 0;
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

		return true;
	}

	socket socket::accept()
	{
		if(!valid())
			return socket();

		socket_internal *si = socket_internal::get(mInternal);

		socket ret(af_inet); // TODO: determine from current socket?

		iocp_async_state state;
		state.thread = uthread::current();
		
		char buf[sizeof(sockaddr_in)*2 + 32];

		if(!si->acceptEx)
		{
			GUID gufn = WSAID_ACCEPTEX;
			DWORD dwBytes;

			if(WSAIoctl(si->handle,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&gufn, sizeof(gufn), &si->acceptEx, sizeof(si->acceptEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR)
			{
				ret.close();
				return std::move(ret);
			}
		}
		
		uthread::current()->suspend([&](){
			if(si->acceptEx(si->handle, (SOCKET)ret.handle(), buf,
				sizeof(buf) - ((sizeof(sockaddr_in) + 16) * 2),
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
				&state.amount, &state.overlapped) == FALSE)
			{
				int error = WSAGetLastError();
				if(error != WSA_IO_PENDING)
				{
					state.error = error;
					state.thread->resume();
				}
			}
		});

		if(state.error)
			ret.close();

		return std::move(ret);
	}

	void socket::close()
	{
		socket_internal *si = socket_internal::get(mInternal);

		if(si->handle != INVALID_SOCKET)
		{
			closesocket(si->handle);
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

			DWORD flags = 0;
			WSABUF buffers;
			buffers.buf = (CHAR*)_buffer;
			buffers.len = (ULONG)_amt;
			
			try
			{
				uthread::current()->suspend([&](){
					WSARecv(si->handle, &buffers, 1, &state.amount,
						&flags, &state.overlapped, NULL);
				});
			}
			catch(std::exception const&)
			{
				CancelIoEx((HANDLE)si->handle, &state.overlapped);
				throw;
			}

			int err = state.error;
			if(err)
			{
				std::cerr << "Recv Err: " << WSAGetLastError() << std::endl;
				CancelIoEx((HANDLE)si->handle, &state.overlapped);
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

			DWORD flags = 0;
			WSABUF buffers;
			buffers.buf = (CHAR*)_buffer;
			buffers.len = (ULONG)_amt;
			
			try
			{
				uthread::current()->suspend([&](){
					WSASend(si->handle, &buffers, 1, &state.amount,
						flags, &state.overlapped, NULL);
				});
			}
			catch(std::exception const&)
			{
				CancelIoEx((HANDLE)si->handle, &state.overlapped);
				throw;
			}

			int err = state.error;
			if(err)
			{
				std::cerr << "WERR: " << GetLastError() << std::endl;
				CancelIoEx((HANDLE)si->handle, &state.overlapped);
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
