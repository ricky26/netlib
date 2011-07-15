#include "netlib/thread.h"
#include "netlib/uthread.h"
#include <exception>
#include <iostream>
#include <Windows.h>

// Magic Microsoft Constants.
#define CLR_EXCEPTION_CODE 0xE0434F4D
#define CPP_EXCEPTION_CODE 0xE06D7363

namespace netlib
{
	//
	// thread_impl
	//

	class thread_impl: public thread
	{
	public:
		thread_impl(thread_fn_t _fn, void *_arg)
			: handle(NULL), function(_fn), argument(_arg)
			, protection(0)
		{
		}

		thread_impl(HANDLE _h)
			: handle(_h), function(NULL), argument(NULL)
			, protection(0)
		{
		}

		static void setup()
		{
			AddVectoredExceptionHandler(TRUE, vectored_handler);

			if(!current)
				current = new thread_impl(GetCurrentThread());
			else
				uthread::enable_uthread();
		}

		static void cleanup()
		{
			if(current)
			{
				thread_impl *ti = current;
				current = NULL;

				//delete ti;
			}
		}

		static LONG CALLBACK vectored_handler(EXCEPTION_POINTERS *_ex)
		{
			int p = 0;
			if(current)
				p += current->protection;

			uthread::handle_t c = uthread::current();
			if(c.get())
				p += c->protection();

			if(current && IsDebuggerPresent() && p == 0)
				return EXCEPTION_CONTINUE_SEARCH;

			switch(_ex->ExceptionRecord->ExceptionCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				throw std::exception("access violation");
				break;

			case EXCEPTION_STACK_OVERFLOW:
				throw std::exception("stack overflow");
				break;

			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				throw std::exception("integer division by zero");
				break;

			case EXCEPTION_BREAKPOINT:
				throw std::exception("breakpoint reached");
				break;
			
			case CPP_EXCEPTION_CODE: // C++ exceptions can be dealt with.
				break;

			default:
				throw std::exception("low level exception");
				break;
			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static __declspec(thread) thread_impl *current;
		thread_fn_t function;
		void *argument;
		HANDLE handle;
		int protection;
	};

	thread_impl *thread_impl::current = NULL;

	//
	// thread
	//

	thread::thread()
	{
	}

	thread::~thread()
	{
	}

	bool thread::join()
	{
		return true;
	}

	bool thread::suspend()
	{
		return true;
	}

	bool thread::resume()
	{
		return true;
	}

	int thread::protection() const
	{
		return ((thread_impl*)this)->protection;
	}

	int thread::protect()
	{
		return ++thread_impl::current->protection;
	}

	int thread::unprotect()
	{
		int ret = thread_impl::current->protection;
		if(ret > 0)
		{
			ret--;
			thread_impl::current->protection = ret;
		}

		return ret;
	}
	
	thread::handle_t thread::current()
	{
		return thread_impl::current;
	}

	static DWORD WINAPI ThreadProc(void *_param)
	{
		thread_impl *ths = (thread_impl*)_param;
		ths->current = ths;
		
		int ret = 0;

		thread_impl::setup();
		try
		{
			ths->function(ths->argument);
		}
		catch(std::exception const& _e)
		{
			std::cerr << "Exception occurred in thread "
				<< ths << ": " << _e.what() << std::endl;
		}
		thread_impl::cleanup();

		return (DWORD)ret;
	}

	thread::handle_t thread::create(thread_fn_t _fn, void *_arg)
	{
		thread_impl *ret = new thread_impl(_fn, _arg);
		HANDLE hThread = CreateThread(NULL, 0, &ThreadProc, ret, 0, NULL);
		if(!hThread)
			return NULL;

		ret->handle = hThread;
		return ret;
	}

	static void create_void(void *_arg)
	{
		((thread::void_fn_t)_arg)();
	}

	thread::handle_t thread::create(void_fn_t _fn)
	{
		return create(create_void, _fn);
	}

	bool thread::init()
	{
		thread_impl::setup();
		return true;
	}

	void thread::shutdown()
	{
		thread_impl::cleanup();
	}
}