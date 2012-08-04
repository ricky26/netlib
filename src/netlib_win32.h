#include "netlib/netlib.h"
#include "netlib/uthread.h"
#include <Windows.h>

#pragma once

namespace netlib
{
	extern HANDLE gCompletionPort;

	//
	// file_async_state
	//

	struct iocp_async_state;
	typedef void (*iocp_async_state_handler_t)(iocp_async_state *_state);

	struct iocp_async_state
	{
		OVERLAPPED overlapped;
		DWORD error;
		DWORD amount;
		uthread::handle_t thread;

		iocp_async_state()
		{
			memset(&overlapped, 0, sizeof(overlapped));
			error = 0;
			amount = 0;
			thread = nullptr;
		}

		iocp_async_state(LARGE_INTEGER &_pos)
		{
			memset(&overlapped, 0, sizeof(overlapped));
			error = 0;
			amount = 0;
			thread = nullptr;
			
			overlapped.Offset = _pos.LowPart;
			overlapped.OffsetHigh = _pos.HighPart;
		}
	};
}
