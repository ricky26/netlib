#include "netlib/uthread.h"
#include "netlib/thread.h"
#include <list>
#include <iostream>
#include <exception>

#include <Windows.h>

namespace netlib
{
	static __declspec(thread) scheduler *gThreadScheduler = NULL;
	static __declspec(thread) PVOID uthread_exception_redir;

	extern "C" void __stdcall _CxxThrowException(void* obj, void* info);

	static uthread_impl *current()
	{
		return (uthread_impl*)GetFiberData();
	}

	//
	// uthread_impl
	// 

	class uthread_impl: public uthread
	{
	public:
		uthread_impl()
		{
			acquire();

			mScheduler = NULL;
			mProtection = 0;
			mThread = thread::current();
			mSuspended = mDead = false;

			fiber = NULL;
			start = NULL;
			argument = NULL;
			running = TRUE;
		}

		uthread_impl(uthread_start_t _start, void *_param)
		{
			acquire();

			mScheduler = NULL;
			mProtection = 0;
			mThread = NULL;
			mSuspended = mDead = false;

			fiber = NULL;
			start = _start;
			argument = _param;
			running = FALSE;
		}

		static void after_swap();

		BOOL running;
		void *fiber;
		uthread_start_t start;
		void *argument;
	};
	
	//
	// uthread
	//

	uthread::uthread()
	{
	}

	uthread::~uthread()
	{
	}

	
	void uthread::destroy()
	{
		acquire(); // Make sure it's not destroyed again.

		uthread_impl *ui = static_cast<uthread_impl*>(this);
		mScheduler->unschedule(this);
		delete ui;
	}

	uthread::handle_t uthread::current()
	{
		return ::netlib::current();
	}
	
	static __declspec(thread) uthread_impl *swapped_from, *swapped_to;
	void uthread_impl::after_swap()
	{
		if(swapped_to->suspended())
		{
			swapped_to->mSuspended = false;
			swapped_to->acquire();
		}

		swapped_from->release();

		if(swapped_from->dead())
		{
			swapped_from->scheduler()->unschedule(swapped_from);
			swapped_from->release();
		}

		InterlockedCompareExchangeRelease((LONG*)&swapped_from->running, FALSE, TRUE);

		if(swapped_to->mRun)
		{
			swapped_to->mRun();
			swapped_to->mRun = run_t();
		}
		
		swapped_to->release();
		swapped_from->release();
	}
		
	bool uthread::swap(uthread *_other)
	{
		if(_other == ::netlib::current())
			return true;

		swapped_from = ::netlib::current();
		swapped_to = (uthread_impl*)_other;

		swapped_from->acquire();
		swapped_to->acquire();

		if(swapped_to->dead())
			throw std::exception("Can't swap to dead thread!");
		
		BOOL ok = InterlockedCompareExchangeAcquire((LONG*)&swapped_to->running, TRUE, FALSE);
		if(ok != FALSE)
			return false;

		swapped_to->scheduler()->unschedule(swapped_to);
		if(!swapped_from->suspended())
			swapped_from->scheduler()->schedule(swapped_from);

		SwitchToFiber(swapped_to->fiber);
		uthread_impl::after_swap();
		return true;
	}

	static void uthread_fiber_start(void *_param)
	{
		uthread_impl::after_swap();
		
		uthread_impl *impl = static_cast<uthread_impl*>(_param);
		try
		{
			impl->start(impl->argument);
		}
		catch(std::exception const& _e)
		{
			std::cerr << "Exception occurred in uthread " 
				<< impl << ": " << _e.what() << std::endl;

			if(impl->protection() <= 0 && IsDebuggerPresent())
				throw;
		}
		catch(...)
		{
			std::cerr << "Unknown exception occurred in uthread "
				<< impl << std::endl;

			if(impl->protection() <= 0 && IsDebuggerPresent())
				throw;
		}

		uthread::exit();
	}

	uthread::handle_t uthread::create(uthread_start_t _start, void *_param)
	{
		uthread_impl *cur = ::netlib::current();
		uthread_impl *thr = new uthread_impl(_start, _param);

		void *fib = CreateFiber(0, (LPFIBER_START_ROUTINE)&uthread_fiber_start, thr);
		if(!fib)
			return NULL;

		thr->fiber = fib;
		thr->schedule(cur->scheduler());
		return thr;
	}
	
	void uthread::exit()
	{
		uthread_impl *impl = ::netlib::current();
		impl->mDead = true;
		impl->suspend();

		throw std::exception("Dead uthread resumed!");
	}

	bool uthread::init()
	{
		enable_uthread();
		return true;
	}

	void uthread::shutdown()
	{
		// TODO: this needs to be done
		// for every thread. >_>
		if(gThreadScheduler)
			delete gThreadScheduler;
	}
	
	void uthread::enable_uthread()
	{
		gThreadScheduler = new netlib::scheduler();

		uthread_impl *impl = new uthread_impl();
		void *f = ConvertThreadToFiber(impl);
		if(!f)
		{
			// TODO: Hold pointer for deletion.
			delete impl;
			return;
		}

		impl->fiber = f;
		impl->mScheduler = gThreadScheduler;
		impl->mPosition = gThreadScheduler->mScheduled.end();
	}
}
