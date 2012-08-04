#include "netlib/netlib.h"
#include "netlib/thread.h"
#include "netlib/uthread.h"
#include "netlib/socket.h"
#include "netlib/pipe.h"
#include "netlib/file.h"
#include "netlib/module.h"
#include "netlib/exception.h"
#include "netlib/win32.h"
#include "netlib_win32.h"
#include <unordered_map>
#include <iostream>
#include <cstdlib>

namespace netlib
{	
	static _declspec(thread) uint64_t gTimeBase = 0;
	static _declspec(thread) uint64_t gCPUFreq = 0;
	static _declspec(thread) uint64_t gTime = 0;

	HANDLE gCompletionPort = nullptr;
	static module gNetlibModule;

	typedef std::unordered_map<UINT, message_handler_t> msg_map_t;
	static msg_map_t gMessageMap;

	static void default_unhandled()
	{
		std::cerr << "Unhandled exception!" << std::endl;

		try
		{
			uthread::current()->exit();
		}
		catch(...)
		{
			try
			{
				thread::exit();
			}
			catch(...)
			{
				netlib::exit(-1);
			}
		}
	}

	static LONG WINAPI UnhandledExceptionHandler(
		__in struct _EXCEPTION_POINTERS *ExceptionInfo
    )
	{
		default_unhandled();
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	NETLIB_API module netlib_module()
	{
		return gNetlibModule;
	}

	NETLIB_API module main_module()
	{
		return module(GetModuleHandle(NULL));
	}

#ifndef NETLIB_STATIC
	extern "C"
	BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
	{
		if(dwReason = DLL_PROCESS_ATTACH)
			gNetlibModule = module((void*)hInstance);

		return TRUE;
	}
#endif

	static inline void setup_time()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&gCPUFreq);
		QueryPerformanceCounter((LARGE_INTEGER*)&gTimeBase);
	}

	static inline bool update_time()
	{
		if(gCPUFreq <= 0) setup_time();

		uint64_t count = 0;
		if(QueryPerformanceCounter((LARGE_INTEGER*)&count) != TRUE)
			return false;

		if(count < gTimeBase)
			return false;

		count -= gTimeBase;
		gTime = (count * 1000000) / gCPUFreq;

		uthread::wake_sleepers();
		return true;
	}

	void endPeriod()
	{
		timeEndPeriod(1);
	}

	static void handle_quit(UINT _msg, LPARAM _lparm, WPARAM _wparm)
	{
		throw quit_exception((int)_wparm);
	}

	static void shutdown()
	{
		file::shutdown();
		pipe::shutdown();
		socket::shutdown();
		uthread::shutdown();
		thread::shutdown();

		if(gCompletionPort)
		{
			CloseHandle(gCompletionPort);
			gCompletionPort = nullptr;
		}

		CoUninitialize();
	}

	NETLIB_API bool init()
	{
		register_message_handler(WM_QUIT, handle_quit);

		atexit(shutdown);	
		set_unexpected(default_unhandled);
		SetUnhandledExceptionFilter(UnhandledExceptionHandler);
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		
		// Used in static netlib.
		if(!gNetlibModule.valid())
			gNetlibModule = module(GetModuleHandle(NULL));

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

		timeBeginPeriod(1);
		atexit(endPeriod);
		setup_time();

		idle(false);
		return true;
	}

	NETLIB_API void exit(int _val)
	{
		PostQuitMessage(_val);
	}

	NETLIB_API bool process_io_single(int _timeout)
	{
		DWORD numDone;
		ULONG_PTR key;
		iocp_async_state *state = nullptr;
		DWORD wait;

		if(_timeout < 0)
			wait = INFINITE;
		else
			wait = (DWORD)_timeout;

		BOOL ret = GetQueuedCompletionStatus(gCompletionPort, &numDone, &key, 
			(OVERLAPPED**)&state, wait);
		if(ret == FALSE && !state)
			return false;

		if(ret)
			state->error = 0;
		else
			state->error = GetLastError();
		
		state->amount = numDone;
		state->thread->resume();
		return true;
	}

	NETLIB_API void process_io(bool _can_block)
	{
		for(;;)
		{
			int timeout = 0;
			if(_can_block)
			{
				_can_block = false;

				// calculate deadline
				uint64_t dl, t=time();
				if(!uthread::deadline(dl))
					timeout = -1;
				else if(dl > t)
					timeout = (int)((dl-t)/1000);
			}

			if(!process_io_single(timeout))
				break;
		}
	}

	NETLIB_API bool process_message()
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE)
		{
			if(!msg.hwnd)
			{
				auto it = gMessageMap.find(msg.message);
				if(it != gMessageMap.end())
					it->second(msg.message, msg.lParam, msg.wParam);
				return msg.message != WM_QUIT;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
			return true;
		}

		return false;
	}

	NETLIB_API void process_messages()
	{
		while(process_message());
	}

	NETLIB_API void update()
	{
		update_time();
	}

	NETLIB_API void idle(bool _can_block)
	{
		update();
		process_messages();
		process_io(_can_block);
	}

	NETLIB_API void idle_slave(bool _can_block)
	{
		update();
		process_io(_can_block);
	}

	NETLIB_API int run_main_loop()
	{
		try
		{
			for(;;) { idle(!uthread::schedule()); }
		}
		catch(quit_exception const& _e)
		{
			return _e.value();
		}
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
	
	NETLIB_API HANDLE completion_port()
	{
		return gCompletionPort;
	}

	NETLIB_API uint64_t time()
	{
		return gTime;
	}
}