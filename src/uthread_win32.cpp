#include "netlib/uthread.h"
#include "netlib/thread.h"
#include <list>
#include <iostream>
#include <exception>

#include <Windows.h>

namespace netlib
{
	static NETLIB_THREAD uthread_impl *dry_thread = nullptr;

	static inline uthread_impl *current()
	{
		return (uthread_impl*)GetFiberData();
	}

	//
	// uthread_impl
	// 

	class uthread_impl: public uthread
	{
	public:
		uthread_impl(uthread_start_t _start=nullptr, void *_param=nullptr)
			: fiber(nullptr),
				start(_start),
				argument(_param),
				next(this),
				prev(this)
		{
			acquire();
		}

		inline bool single() const
		{
			return next == prev && next == this;
		}

		void insert(uthread_impl *_prev)
		{
			if(_prev->next == this)
				return;

			if(!single())
				remove();

			uthread_impl *n = _prev->next;
			prev = _prev;
			next = n;

			prev->next = next->prev = this;
		}

		void remove()
		{
			uthread_impl *p = prev, *n = next;
			p->next = n;
			n->prev = p;

			prev = next = this;
		}

		void destroy() override
		{
			remove();
			DeleteFiber(fiber);
			fiber = nullptr;
			uthread::destroy();
		}

		void do_run()
		{
			if(mRun)
			{
				run_t r = mRun;
				mRun = run_t();

				r();
			}
		}

		void swap_next();
		void after_swap();

		// Linked list
		uthread_impl *prev, *next;

		// uThread data
		void *fiber;
		uthread_start_t start;
		void *argument;
	};
	
	//
	// uthread
	//

	uthread::handle_t uthread::current()
	{
		return ::netlib::current();
	}

	void uthread_impl::swap_next()
	{
		SwitchToFiber(next->fiber);
		::netlib::current()->after_swap();
	}
	
	void uthread_impl::after_swap()
	{
		uthread_impl *p = prev;

		if(p->mSuspended)
		{
			p->remove();
			p->do_run();
			p->release();
		}

		if(mDead)
			throw std::runtime_error("Dead uthread resumed!");
		else
			do_run();
	}
		
	bool uthread::swap(uthread *_other)
	{
		uthread_impl *cur = ::netlib::current();
		if(_other == cur)
			return true;
		
		uthread_impl *impl = static_cast<uthread_impl*>(_other);
		if(impl->mSuspended)
		{
			impl->mSuspended = false;
			impl->acquire();
		}

		impl->insert(cur);
		cur->swap_next();
		return true;
	}

	// TODO: if this works, then it could do with being
	// not-boolean. -- Ricky26
	bool uthread::schedule()
	{
		uthread_impl *cur = ::netlib::current();

		if(cur->single())
			swap(dry_thread);
		else
			cur->swap_next();

		return true;
	}

	void uthread::suspend()
	{
		uthread_impl *ths = static_cast<uthread_impl*>(this);

		mSuspended = true;
		if(::netlib::current() == ths)
		{
			if(!schedule())
				throw std::exception("Can't suspend last uthread!");
		}
		else
			ths->remove();
	}

	void uthread::resume(bool _swap)
	{
		if(_swap)
			swap(this);
		else
		{
			mSuspended = false;
			uthread_impl *ths = static_cast<uthread_impl*>(this);
			ths->acquire();
			ths->insert(::netlib::current());
		}
	}

	struct uthread_safestart
	{
		~uthread_safestart()
		{
			uthread::current()->exit();
		}
	};

	static void uthread_fiber_start(void *_param)
	{		
		uthread_impl *impl = static_cast<uthread_impl*>(_param);
		uthread_safestart ss;

		impl->after_swap();
		impl->start(impl->argument);
	}

	uthread_impl *uthread_create(uthread::uthread_start_t _start, void *_param)
	{
		uthread_impl *cur = ::netlib::current();
		handle<uthread_impl> thr = new uthread_impl(_start, _param);

		// 1 <-- creates the smallest possible stack, it can grow, worry not.
		void *fib = CreateFiber(1, (LPFIBER_START_ROUTINE)&uthread_fiber_start, thr.get());
		if(!fib)
			return NULL;

		thr->fiber = fib;
		return thr.get();
	}

	uthread::handle_t uthread::create(uthread_start_t _start, void *_param)
	{
		handle<uthread> thr = uthread_create(_start, _param);
		swap(thr.get());
		return thr;
	}

	bool uthread::init()
	{
		return true;
	}

	void uthread::shutdown()
	{
	}

	static void uthread_dry(void *)
	{
		for(;;)
		{
			// TODO: figure out how to deal with
			// messages _AND_ IO whilst blocking
			// in the kernel. :<
			uthread_impl *impl = netlib::current();
			while(impl->single()) idle(false);
			impl->suspend();
		}
	}
	
	void uthread::enter_thread()
	{
		uthread_impl *impl = new uthread_impl();
		void *f = ConvertThreadToFiber(impl);
		if(!f)
		{
			delete impl;
			DeleteFiber(f);
			return;
		}

		impl->fiber = f;

		if(!dry_thread)
		{
			dry_thread = uthread_create(uthread_dry, nullptr);
			dry_thread->mSuspended = true;
		}

		enter_thread_common();
	}

	void uthread::exit_thread()
	{
		if(dry_thread)
		{
			uthread_impl *dt = dry_thread;
			dry_thread = nullptr;

			dt->release();
		}

		exit_thread_common();
	}
}
