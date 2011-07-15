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

			prev = next = NULL;
			fiber = NULL;
			start = NULL;
			argument = NULL;
			isMain = true;
			isDead = false;
			suspended = false;
			protection = 0;
			scheduler = NULL;
			running = TRUE;
			exception = NULL;
			exception_redirect = NULL;
		}

		uthread_impl(uthread_start_t _start, void *_param)
		{
			acquire();

			prev = next = NULL;
			fiber = NULL;
			start = _start;
			argument = _param;
			isMain = false;
			isDead = false;
			suspended = false;
			protection = 0;
			scheduler = NULL;
			running = FALSE;
			exception = NULL;
			exception_redirect = NULL;
		}

		void unlink()
		{
			netlib::scheduler *sch = scheduler;
			if(sch)
			{
				WaitForSingleObject(
					(HANDLE)sch->mInternal, INFINITE);

				if(scheduler->mLast == this)
				{
					if(prev == this)
						sch->mFirst = sch->mLast = NULL;
					else
						sch->mLast = prev;
				}
				
				prev->next = next;
				next->prev = prev;
				scheduler = NULL;
				ReleaseMutex((HANDLE)sch->mInternal);
			}
		}

		void insert(uthread_impl *_after)
		{
			netlib::scheduler *sch = _after->scheduler;

			WaitForSingleObject((HANDLE)sch->mInternal, INFINITE);

			unlink();

			prev = _after;
			next = prev->next;

			prev->next = this;
			next->prev = this;

			sch->mLast = this;
			scheduler = sch;

			ReleaseMutex((HANDLE)sch->mInternal);
		}

		void swap_with(uthread_impl *_ui)
		{
			/*netlib::scheduler *sch = scheduler;
			WaitForSingleObject((HANDLE)sch->mInternal, INFINITE);

			uthread_impl *p = prev;
			uthread_impl *n = next;

			prev = _ui->prev;
			next = _ui->next;
			prev->next = this;
			next->prev = this;

			p->next = _ui;
			n->prev = _ui;
			_ui->prev = p;
			_ui->next = n;

			ReleaseMutex((HANDLE)sch->mInternal);*/
		}

		BOOL running;
		void *fiber;
		uthread_start_t start;
		void *argument;
		bool isMain;
		bool suspended;
		bool isDead;
		int protection;
		netlib::scheduler *scheduler;
		void *exception;
		void *throw_info;
		uthread_impl *next, *prev;
		uthread_impl *swapped_with;
		uthread_impl *exception_redirect;
	};
	
	//
	// scheduler
	//

	scheduler::scheduler()
	{
		mFirst = mLast = NULL;
		mInternal = (void*)CreateMutexA(NULL, FALSE, NULL);
	}

	scheduler::~scheduler()
	{
		CloseHandle((HANDLE)mInternal);
	}

	void scheduler::insert(uthread *_thr)
	{
		uthread_impl *ui = (uthread_impl*)_thr;
		if(ui->scheduler == this)
			return;

		if(!mFirst)
		{
			ui->unlink();
			
			WaitForSingleObject((HANDLE)mInternal, INFINITE);
			mFirst = mLast = ui->next = ui->prev = ui;
			ui->scheduler = this;
			ReleaseMutex((HANDLE)mInternal);
		}
		else
		{
			ui->insert((uthread_impl*)mLast);
		}
	}

	void scheduler::schedule()
	{
		insert(::netlib::current());
		uthread::schedule();
	}

	static void scheduler_worker_thread(scheduler *sch)
	{
		sch->schedule();
		netlib::run_main_loop();
	}

	thread::handle_t scheduler::create_worker_thread()
	{
		return thread::create(scheduler_worker_thread, this);
	}

	void scheduler::create_worker_threads(int _icnt)
	{
		for(int i = 0; i < _icnt; i++)
			create_worker_thread();
	}
	
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
		uthread_impl *ui = static_cast<uthread_impl*>(this);
		delete ui;
	}

	uthread::handle_t uthread::current()
	{
		return ::netlib::current();
	}

	static void uthread_fiber_start(void *_param)
	{
		uthread_impl *impl = static_cast<uthread_impl*>(_param);
		InterlockedCompareExchangeRelease(
			(LONG*)&impl->swapped_with->running, FALSE, TRUE);

		try
		{
			impl->start(impl->argument);
		}
		catch(std::exception const& _e)
		{
			std::cerr << "Exception occurred in fiber " 
				<< impl << ": " << _e.what() << std::endl;

			if(impl->protection <= 0 && IsDebuggerPresent())
				throw _e;
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
		cur->scheduler->insert(thr);
		return thr;
	}

	static void uthread_start_0(void *_ptr)
	{
		uthread::void_fn_t fn = (uthread::void_fn_t)_ptr;
		fn();
	}

	uthread::handle_t uthread::create(void_fn_t _start)
	{
		uthread_impl *cur = ::netlib::current();
		uthread_impl *thr = new uthread_impl(uthread_start_0, _start);

		void *fib = CreateFiber(0, (LPFIBER_START_ROUTINE)&uthread_fiber_start, thr);
		if(!fib)
			return NULL;

		thr->fiber = fib;
		cur->scheduler->insert(thr);
		return thr;
	}
		
	bool uthread::swap(uthread *_other)
	{
		if(_other == ::netlib::current())
			return true;

		static __declspec(thread) uthread_impl *cur, *other;

		cur = ::netlib::current();
		other = (uthread_impl*)_other;
		
		BOOL ok = InterlockedCompareExchangeAcquire((LONG*)&other->running, TRUE, FALSE);
		if(ok != FALSE)
			return false;

		if(other->suspended)
			other->suspended = false;

		other->swapped_with = cur;
		SwitchToFiber(other->fiber);

		InterlockedCompareExchangeRelease((LONG*)&cur->running, FALSE, TRUE);

		if(other->exception)
		{
			void *exc = other->exception;
			void *ti = other->throw_info;

			other->exception = NULL;

			_CxxThrowException(exc, ti);
		}

		return true;
	}

	void uthread::schedule()
	{
		uthread_impl *cur = ::netlib::current();
		if(cur->next == cur)
			goto idle; // Short circuit.

		HANDLE hMutex = (HANDLE)cur->scheduler->mInternal;
		if(WaitForSingleObject(hMutex, 2) != WAIT_TIMEOUT)
		{
			uthread_impl *next = cur->next;
			while(next != cur)
			{
				if(next->isDead)
				{
					uthread_impl *dead = next;
					next = next->next;
				
					dead->unlink();
					dead->release();
					continue;
				}

				if(!next->suspended && next->running == FALSE)
				{
					ReleaseMutex(hMutex);
					if(!swap(next))
					{
						if(WaitForSingleObject(hMutex, 2) == WAIT_TIMEOUT)
							goto idle;

						continue;
					}
				
					return;
				}

				next = next->next;
			}
		}

		ReleaseMutex(hMutex);

idle:
		SleepEx(0, TRUE);
	}

	void uthread::schedule(netlib::scheduler *_sch)
	{
		_sch->insert(this);
	}

	scheduler *uthread::scheduler() const
	{
		return ((uthread_impl*)this)->scheduler;
	}

	void uthread::suspend()
	{
		uthread_impl *impl = ::netlib::current();
		if(impl->isMain)
		{
			std::cerr << "Can't suspend main!" << std::endl;
			return;
		}

		impl->suspended = true;
		schedule();
	}

	void uthread::resume()
	{
		uthread_impl *impl = static_cast<uthread_impl*>(this);
		if(impl->suspended)
		{
			impl->suspended = false;

			uthread_impl *curr = ::netlib::current();

			if(impl->scheduler == curr->scheduler)
			{
				curr->swap_with(impl);
				swap(impl);
			}
		}
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
		WaitForSingleObject((HANDLE)impl->scheduler->mInternal, INFINITE);
		impl->isDead = 1;
		schedule();
	}

	int uthread::protection() const
	{
		uthread_impl *impl = (uthread_impl*)this;
		return impl->protection;
	}

	int uthread::protect()
	{
		uthread_impl *impl = ::netlib::current();
		return ++impl->protection;
	}

	int uthread::unprotect()
	{
		uthread_impl *impl = ::netlib::current();
		if(impl->protection == 0)
			return 0;

		return --impl->protection;
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
		gThreadScheduler->insert(impl);
	}
}
