#include "netlib/pipe.h"
#include "netlib/socket.h"
#include "netlib_win32.h"
#include <iostream>

#ifndef NETLIB_PIPE_BUFFER
#define NETLIB_PIPE_BUFFER (16*1024) // 16kb
#endif

namespace netlib
{
	//
	// pipe_internal
	//

	struct pipe_internal
	{
		HANDLE handle;
		std::string name;

		pipe_internal(HANDLE _hdl=INVALID_HANDLE_VALUE)
		{
			handle = _hdl;
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
		mInternal = new pipe_internal();
	}

	pipe::pipe(int _handle)
	{
		mInternal = new pipe_internal((HANDLE)_handle);
	}

	pipe::pipe(pipe &_p)
	{
		mInternal = _p.mInternal;
		_p.mInternal = new pipe_internal();
		
		if(mInternal)
		{
			pipe_internal::get(_p.mInternal)->name
				= pipe_internal::get(mInternal)->name;
		}
		else
			mInternal = new pipe_internal();
	}

	pipe::~pipe()
	{
		if(mInternal)
		{
			pipe_internal *pi = pipe_internal::get(mInternal);
			mInternal = NULL;

			if(pi->handle != INVALID_HANDLE_VALUE)
				CloseHandle(pi->handle);

			delete pi;
		}
	}

	bool pipe::valid() const
	{
		if(!mInternal)
			return false;

		register pipe_internal *pi = pipe_internal::get(mInternal);
		return pi->handle != INVALID_HANDLE_VALUE;
	}

	int pipe::handle() const
	{
		if(!mInternal)
			return (int)INVALID_HANDLE_VALUE;

		return (int)pipe_internal::get(mInternal)->handle;
	}

	int pipe::release()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		int hdl = (int)pi->handle;
		pi->handle = INVALID_HANDLE_VALUE;
		return hdl;
	}
		
	bool pipe::open(std::string const& _pipe)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle != INVALID_HANDLE_VALUE)
			return false;

		std::string name = "\\\\.\\pipe\\" + _pipe;
		HANDLE hPipe = CreateFileA(name.c_str(),
			GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED, NULL);
		if(hPipe == INVALID_HANDLE_VALUE)
			return false;

		// Add the pipe to the completion port
		if(CreateIoCompletionPort(hPipe,
			gCompletionPort, 0, 0) != gCompletionPort)
		{
			std::cerr << "ERR: Failed to add pipe to completion port." << std::endl;
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
		}

		pi->name = name;
		pi->handle = hPipe;
		return true;
	}

	bool pipe::create(std::string const& _pipe)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle != INVALID_HANDLE_VALUE)
			return false;

		std::string name = "\\\\.\\pipe\\" + _pipe;
		HANDLE hPipe = CreateNamedPipeA(name.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
			PIPE_UNLIMITED_INSTANCES, NETLIB_PIPE_BUFFER, NETLIB_PIPE_BUFFER,
			0, NULL);
		if(hPipe == INVALID_HANDLE_VALUE)
			return false;

		pi->handle = hPipe;
		pi->name = name;
		return true;
	}

	pipe pipe::accept()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(pi && pi->name.empty())
			return pipe();

		HANDLE hPipe = pi->handle;
		pi->handle = INVALID_HANDLE_VALUE;
		if(hPipe == INVALID_HANDLE_VALUE)
			hPipe = CreateNamedPipeA(pi->name.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
			PIPE_UNLIMITED_INSTANCES, NETLIB_PIPE_BUFFER, NETLIB_PIPE_BUFFER,
			0, NULL);

		if(hPipe != INVALID_HANDLE_VALUE)
		{
			// Add the pipe to the completion port
			if(CreateIoCompletionPort(hPipe,
				gCompletionPort, 0, 0) != gCompletionPort)
			{
				std::cerr << "ERR: Failed to add pipe to completion port." << std::endl;
				CloseHandle(hPipe);
				hPipe = INVALID_HANDLE_VALUE;
			}
		}

		if(hPipe != INVALID_HANDLE_VALUE)
		{
			iocp_async_state state;
			state.thread = uthread::current();
			ConnectNamedPipe(hPipe, &state.overlapped);

			bool success = false;
			switch(GetLastError())
			{
			case ERROR_PIPE_CONNECTED:
				// We connected, yay!
				success = true;
				break;

			case ERROR_IO_PENDING:
				uthread::suspend();
				success = SUCCEEDED(state.error);

				if(!success)
					std::cerr << "Error: " << state.error << std::endl;
				break;

			default:
				std::cerr << "Error: " << GetLastError() << std::endl;
				break;
			}

			if(!success)
			{
				CloseHandle(hPipe);
				hPipe = INVALID_HANDLE_VALUE;
			}
		}
		
		pipe ret;
		pipe_internal *ri = pipe_internal::get(ret.mInternal);
		ri->handle = hPipe;
		ri->name = pi->name;
		return ret;
	}

	void pipe::close()
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle == INVALID_HANDLE_VALUE)
			return;

		CloseHandle(pi->handle);
		pi->handle = INVALID_HANDLE_VALUE;
	}

	size_t pipe::read(void *_buffer, size_t _amt)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle == INVALID_HANDLE_VALUE)
			return 0;

		iocp_async_state state;
		state.thread = uthread::current();

		if(ReadFile(pi->handle, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
			|| GetLastError() == ERROR_IO_PENDING)
		{
			uthread::suspend();

			if(state.error != 0)
				return 0;
		}
		else
		{
			std::cerr << "Recv Err: " << GetLastError() << std::endl;
			return 0;
		}

		return state.amount;
	}

	size_t pipe::write(const void *_buffer, size_t _amt)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle == INVALID_HANDLE_VALUE)
			return 0;

		iocp_async_state state;
		state.thread = uthread::current();

		if(WriteFile(pi->handle, _buffer, _amt, &state.amount, &state.overlapped) == TRUE
			|| GetLastError() == ERROR_IO_PENDING)
		{
			uthread::suspend();

			if(state.error != 0)
				return 0;
		}
		else
		{
			std::cerr << "WERR: " << GetLastError() << std::endl;
			return 0;
		}

		return state.amount;
	}
		
	socket pipe::read()
	{
		HANDLE handle;

		if(!read_block(&handle, sizeof(handle)))
			return socket();

		return socket((int)handle);
	}

	bool pipe::write(socket &_sock)
	{
		pipe_internal *pi = pipe_internal::get(mInternal);
		if(!pi || pi->handle == INVALID_HANDLE_VALUE)
			return false;

		DWORD flags;
		if(!GetNamedPipeInfo(pi->handle, &flags, NULL, NULL, NULL))
			return false;

		HANDLE localProc = GetCurrentProcess();
		HANDLE remoteProc;
		ULONG remoteProcId;

		BOOL ret;
		if(flags & PIPE_SERVER_END)
			ret = GetNamedPipeClientProcessId(pi->handle, &remoteProcId);
		else
			ret = GetNamedPipeServerProcessId(pi->handle, &remoteProcId);
		if(!ret)
			return false;

		remoteProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, remoteProcId);
		if(remoteProc == INVALID_HANDLE_VALUE)
			return false;

		HANDLE outHandle;
		if(!DuplicateHandle(localProc, (HANDLE)_sock.handle(), remoteProc, &outHandle,
			GENERIC_READ | GENERIC_WRITE, FALSE, DUPLICATE_SAME_ACCESS))
			return false;

		return write_block(&outHandle, sizeof(outHandle));
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
