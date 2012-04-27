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
		{
		}

		thread_impl(HANDLE _h)
			: handle(_h), function(NULL), argument(NULL)
		{
		}

		static void setup()
		{
			_set_se_translator(&seh_handler);
			AddVectoredExceptionHandler(FALSE, vectored_handler);

			if(!current)
				current = new thread_impl(GetCurrentThread());
			else
				uthread::enable_uthread();

			current->exc = nullptr;
			current->acquire();
		}

		static void cleanup()
		{
			if(current)
				current->release();
		}

		static void kill_thread()
		{
			try
			{
				uthread::exit();
			}
			catch(std::exception const&)
			{
				try
				{
					thread::exit();
				}
				catch(std::exception const&)
				{
					netlib::exit(-1);
				}
			}
		}

		static LONG CALLBACK vectored_handler(EXCEPTION_POINTERS *_ex)
		{
			if(!IsDebuggerPresent())
				return EXCEPTION_CONTINUE_SEARCH;

			DebugBreak();
			std::cerr << "Stepped over exception!" << std::endl;
			
#ifdef NETLIB_X64
			_ex->ContextRecord->Rip = (DWORD64)&kill_thread;
#else
			_ex->ContextRecord->Eip = (DWORD)&kill_thread;
#endif
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		static /*LONG CALLBACK*/ void seh_handler(unsigned int _code,
			EXCEPTION_POINTERS *_ex)
		{
			switch(_code)
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
			
			/*default:
				return EXCEPTION_CONTINUE_SEARCH;*/
			}

			//return EXCEPTION_EXECUTE_HANDLER;
		}

		static __declspec(thread) thread_impl *current;
		thread_fn_t function;
		void *argument;
		HANDLE handle;
		EXCEPTION_POINTERS *exc;
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
		DWORD ret = WaitForSingleObject(((thread_impl*)this)->handle, INFINITE);
		return ret == WAIT_OBJECT_0;
	}
		
	void thread::exit()
	{
		if(!TerminateThread(GetCurrentThread(), -1))
			throw std::runtime_error("failed to kill thread");
	}

	void thread::sleep(int _ms)
	{
		Sleep(_ms);
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
		ths->release(); // acquire in create
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
		ret->acquire();
		return ret;
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

	//
	// thread_condition
	//

	thread_condition::thread_condition()
	{
		InitializeConditionVariable((CONDITION_VARIABLE*)&mInternal);
	}

	thread_condition::~thread_condition()
	{
	}

	//
	// mutex
	//

	mutex::mutex()
	{
		mInternal = new CRITICAL_SECTION;
		InitializeCriticalSection((CRITICAL_SECTION*)mInternal);
	}

	mutex::~mutex()
	{
		DeleteCriticalSection((CRITICAL_SECTION*)mInternal);
		delete (CRITICAL_SECTION*)mInternal;
	}

	bool mutex::try_lock()
	{
		return TryEnterCriticalSection((CRITICAL_SECTION*)mInternal) == TRUE;
	}

	void mutex::lock()
	{
		EnterCriticalSection((CRITICAL_SECTION*)mInternal);
	}

	void mutex::unlock()
	{
		LeaveCriticalSection((CRITICAL_SECTION*)mInternal);
	}

	void mutex::wait(thread_condition &_con)
	{
		SleepConditionVariableCS((CONDITION_VARIABLE*)&_con.mInternal,
			(CRITICAL_SECTION*)mInternal, INFINITE);
	}

	//
	// rw_lock
	//

	rw_lock::rw_lock()
	{
		InitializeSRWLock((SRWLOCK*)&mInternal);
	}

	rw_lock::~rw_lock()
	{
	}

	bool rw_lock::try_lock_read()
	{
		return TryAcquireSRWLockShared((SRWLOCK*)&mInternal) == TRUE;
	}

	void rw_lock::lock_read()
	{
		AcquireSRWLockShared((SRWLOCK*)&mInternal);
	}

	void rw_lock::unlock_read()
	{
		ReleaseSRWLockShared((SRWLOCK*)&mInternal);
	}

	bool rw_lock::try_lock_write()
	{
		return TryAcquireSRWLockExclusive((SRWLOCK*)&mInternal) == TRUE;
	}

	void rw_lock::lock_write()
	{
		AcquireSRWLockExclusive((SRWLOCK*)&mInternal);
	}

	void rw_lock::unlock_write()
	{
		ReleaseSRWLockExclusive((SRWLOCK*)&mInternal);
	}
}