#include "netlib/socket.h"
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
	// socket_state_t
	//

	enum socket_state_t
	{
		socket_idle,
		socket_recv,
		socket_send,
		socket_connect,
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
	
	socket::socket(address_family _af, socket_type _sock, socket_protocol _prot)
	{		
		int af, type, prot;
		if(!getSocketParams(_af, _sock, _prot, af, type, prot))
			mInternal = (void*)INVALID_SOCKET;
		else
			mInternal = (void*)WSASocket(af, type, prot, NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	socket::socket(int _sock)
	{
		mInternal = (void*)_sock;

		if(_sock != INVALID_SOCKET)
		{
			if(CreateIoCompletionPort((HANDLE)_sock,
				gCompletionPort, 0, 0) != gCompletionPort)
			{
				closesocket((SOCKET)_sock);
				mInternal = (void*)INVALID_SOCKET;
			}
		}
	}
	
	socket::socket(socket_constructor_t const& _con)
	{
		mInternal = _con.value;
	}
	
	socket::socket(socket &_other)
	{
		mInternal = _other.mInternal;
		_other.mInternal = (void*)INVALID_SOCKET;
	}

	socket::~socket()
	{
		if(mInternal != (void*)INVALID_SOCKET)
			closesocket((SOCKET)mInternal);
	}
	
	bool socket::valid() const
	{
		return mInternal != (void*)INVALID_SOCKET;
	}
	
	int socket::handle() const
	{
		return (int)mInternal;
	}
	
	int socket::release()
	{
		int ret = (int)mInternal;
		mInternal = (void*)INVALID_SOCKET;
		return ret;
	}
	
	socket_constructor_t socket::returnable_value()
	{
		void *ret = mInternal;
		mInternal = (void*)INVALID_SOCKET;
		return ret;
	}
	
	bool socket::connect(std::string const& _host, int _port)
	{
		if(mInternal == (void*)INVALID_SOCKET)
			return false;

		hostent *he = gethostbyname(_host.c_str());
		if(!he)
			return false;

		// Find ConnectEx pointer!

		if(!ConnectEx)
		{
			GUID gufn = WSAID_CONNECTEX;
			DWORD dwBytes;

			if(WSAIoctl((SOCKET)mInternal,
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

		if(bind((SOCKET)mInternal,
			(sockaddr*)&src_addr, sizeof(src_addr)) != 0)
			return false;

		// Bind to Completion Port
		if(CreateIoCompletionPort((HANDLE)mInternal,
			gCompletionPort, 0, 0) != gCompletionPort)
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

		if(ConnectEx((SOCKET)mInternal, (sockaddr*)&addr,
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
		if(mInternal == (void*)INVALID_SOCKET)
			return false;

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(_port);

		if(::bind((SOCKET)mInternal, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
			return false;
		}

		// Bind to Completion Port
		if(CreateIoCompletionPort((HANDLE)mInternal,
			gCompletionPort, 0, 0) != gCompletionPort)
			return false;

		if(::listen((SOCKET)mInternal, _amt) == SOCKET_ERROR)
			return false;

		if(!AcceptEx)
		{
			GUID gufn = WSAID_ACCEPTEX;
			DWORD dwBytes;

			if(WSAIoctl((SOCKET)mInternal,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&gufn, sizeof(gufn), &AcceptEx, sizeof(AcceptEx),
				&dwBytes, NULL, NULL) == SOCKET_ERROR)
				return false;
		}

		return true;
	}

	socket_constructor_t socket::accept()
	{
		socket ret;

		iocp_async_state state;
		state.thread = uthread::current();
		
		char buf[sizeof(sockaddr_in)*2 + 32];
		
		if(AcceptEx((SOCKET)mInternal, (SOCKET)ret.handle(), buf,
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

		return (void*)ret.release();
	}

	void socket::close()
	{
		if(mInternal != (void*)INVALID_SOCKET)
		{
			CloseHandle((HANDLE)mInternal);
			mInternal = (void*)INVALID_SOCKET;
		}
	}

	size_t socket::read(void *_buffer, size_t _amt)
	{
		if(mInternal != (void*)INVALID_SOCKET)
		{
			iocp_async_state state;
			state.thread = uthread::current();

			int err;
			if(ReadFile((HANDLE)mInternal, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
				|| (err = WSAGetLastError()) == WSA_IO_PENDING)
			{
				uthread::suspend();
				err = state.error;
			}
			CRITICAL_SECTION;
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
		if(mInternal != (void*)INVALID_SOCKET)
		{
			iocp_async_state state;
			state.thread = uthread::current();

			int err;
			if(WriteFile((HANDLE)mInternal, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
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