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
			exception = NULL;
			exception_redirect = NULL;
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
			exception = NULL;
			exception_redirect = NULL;
		}

		static void after_swap();

		BOOL running;
		void *fiber;
		uthread_start_t start;
		void *argument;
		void *exception;
		void *throw_info;
		uthread_impl *swapped_with;
		uthread_impl *exception_redirect;
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
		if(swapped_from->dead())
		{
			swapped_from->scheduler()->unschedule(swapped_from);
			swapped_from->release();
		}
		else
			InterlockedCompareExchangeRelease((LONG*)&swapped_from->running, FALSE, TRUE);

		if(swapped_to->exception)
		{
			void *exc = swapped_to->exception;
			void *ti = swapped_to->throw_info;

			swapped_to->exception = NULL;

			_CxxThrowException(exc, ti);
		}
	}
		
	bool uthread::swap(uthread *_other)
	{
		if(_other == ::netlib::current())
			return true;

		swapped_from = ::netlib::current();
		swapped_to = (uthread_impl*)_other;

		if(swapped_to->dead())
			throw std::exception("Can't swap to dead thread!");
		
		BOOL ok = InterlockedCompareExchangeAcquire((LONG*)&swapped_to->running, TRUE, FALSE);
		if(ok != FALSE)
			return false;

		if(swapped_to->suspended())
			swapped_to->mSuspended = false;

		swapped_to->scheduler()->unschedule(swapped_to);
		if(!swapped_from->suspended())
			swapped_from->scheduler()->schedule(swapped_from);

		swapped_to->swapped_with = swapped_from;
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
			std::cerr << "Exception occurred in fiber " 
				<< impl << ": " << _e.what() << std::endl;

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

	static LONG CALLBACK uthread_exception_redirector(EXCEPTION_POINTERS *_ex)
	{
		uthread_impl *cur = ::netlib::current();
		uthread_impl *next = cur->exception_redirect;
		if(next->exception)
			return EXCEPTION_CONTINUE_SEARCH;

		next->exception = ((char*)_ex->ExceptionRecord->ExceptionInformation[1])+300;
		next->throw_info = (void*)_ex->ExceptionRecord->ExceptionInformation[2];
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void uthread::redirect_exceptions(handle_t _h)
	{
		uthread_impl *cur = ::netlib::current();

		if(_h.get())
		{
			// Enable
			cur->exception_redirect = (uthread_impl*)_h.get();
			uthread_exception_redir = AddVectoredExceptionHandler(
				TRUE, uthread_exception_redirector);
		}
		else
		{
			uthread_impl *next = cur->exception_redirect;

			RemoveVectoredExceptionHandler(uthread_exception_redir);

			cur->exception_redirect = NULL;
			swap(next);
		}
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
