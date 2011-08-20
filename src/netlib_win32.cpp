#include "netlib/netlib.h"
#include "netlib/thread.h"
#include "netlib/uthread.h"
#include "netlib/socket.h"
#include "netlib/pipe.h"
#include "netlib/file.h"
#include "netlib/win32.h"
#include "netlib_win32.h"
#include <unordered_map>

namespace netlib
{	
	HANDLE gCompletionPort = NULL;
	static bool gIsDone;
	static int gRetVal;

	typedef std::unordered_map<int, message_handler_t> msg_map_t;
	static msg_map_t gMessageMap;

	NETLIB_API bool init()
	{
		gIsDone = false;
		gRetVal = 0;

		gCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, (ULONG_PTR)0, 0);
		if(!gCompletionPort)
			return false;

		if(!thread::init())
			return false;

		if(!uthread::init())
			return false;

		if(!socket::init())
			return false;

		if(!pipe::init())
			return false;

		if(!file::init())
			return false;

		return true;
	}

	NETLIB_API void exit(int _val)
	{
		if(!gIsDone)
		{
			gRetVal = _val;
			gIsDone = true;

			file::shutdown();
			pipe::shutdown();
			socket::shutdown();
			uthread::shutdown();
			thread::shutdown();

			if(gCompletionPort)
			{
				CloseHandle(gCompletionPort);
				gCompletionPort = NULL;
			}
		}
	}

	NETLIB_API int exit_value()
	{
		return gRetVal;
	}

	NETLIB_API bool running()
	{
		return !gIsDone;
	}

	NETLIB_API bool think()
	{
		if(gIsDone)
			return false;

		DWORD numDone = 0;
		ULONG_PTR key;
		iocp_async_state *state = NULL;

		while(true)
		{
			if(GetQueuedCompletionStatus(gCompletionPort, &numDone, &key,
				(OVERLAPPED**)&state, 0) == TRUE)
			{
				state->error = 0;
				state->amount = numDone;

				if(state->handler)
					state->handler(state);
				else
					state->thread->resume();
			}
			else if(state)
			{
				state->error = GetLastError();
				state->amount = 0;

				if(state->handler)
					state->handler(state);
				else
					state->thread->resume();
			}
			else
				break;
		}

		if(!uthread::schedule())
			SleepEx(1, TRUE); // TODO: Find a better value for this?

		MSG msg;
		while(PeekMessage(&msg, NULL, 0, 0, 1) == TRUE)
		{
			TranslateMessage(&msg);
			if(!msg.hwnd)
			{
				auto it = gMessageMap.find(msg.message);
				if(it != gMessageMap.end())
					it->second(msg.message, msg.lParam, msg.wParam);
			}
			else
				DispatchMessage(&msg);
		}

		return true;
	}

	NETLIB_API int run_main_loop()
	{
		while(think());
		return exit_value();
	}
	
	NETLIB_API bool register_message_handler(int _msg, message_handler_t const& _hdlr)
	{
		auto it = gMessageMap.find(_msg);
		if(it != gMessageMap.end())
			return false;

		gMessageMap.insert(msg_map_t::value_type(_msg, _hdlr));
		return true;
	}

	NETLIB_API void remove_message_handler(int _msg)
	{
		auto it = gMessageMap.find(_msg);
		if(it != gMessageMap.end())
			gMessageMap.erase(it);
	}
}