#include "netlib/uthread.h"
#include "netlib/thread.h"
#include <list>
#include <iostream>
#include <exception>

#include <Windows.h>

namespace netlib
{
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
			if(!single())
				remove();
			
			acquire();

			uthread_impl *n = _prev->next;
			prev = _prev;
			next = n;

			prev->next = this;
			next->prev = this;
		}

		void remove()
		{
			uthread_impl *p = prev, *n = next;
			p->next = n;
			n->prev = p;

			release();
		}

		void destroy() override
		{
			uthread::destroy();
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
		next->acquire();
		SwitchToFiber(next->fiber);

		::netlib::current()->after_swap();
	}
	
	void uthread_impl::after_swap()
	{
		prev->release();

		if(mRun)
		{
			prev->insert(this);

			mRun();
			mRun = run_t();

			swap_next();
		}
	}
		
	bool uthread::swap(uthread *_other)
	{
		uthread_impl *cur = ::netlib::current();
		if(_other == cur)
			return true;
		
		uthread_impl *impl = static_cast<uthread_impl*>(_other);
		impl->insert(cur);
		cur->swap_next();
		return true;
	}

	bool uthread::schedule()
	{
		uthread_impl *cur = ::netlib::current();
		if(cur->single())
			return false;

		cur->swap_next();
		return true;
	}

	struct uthread_safestart
	{
		uthread_impl *impl;

		uthread_safestart(uthread_impl *_impl): impl(_impl)
		{
			impl->start(impl->argument);
		}

		~uthread_safestart()
		{
			uthread::exit();
		}
	};

	static void uthread_fiber_start(void *_param)
	{		
		uthread_impl *impl = static_cast<uthread_impl*>(_param);
		impl->after_swap();
		uthread_safestart ss(impl);
	}

	uthread::handle_t uthread::create(uthread_start_t _start, void *_param)
	{
		uthread_impl *cur = ::netlib::current();
		uthread_impl *thr = new uthread_impl(_start, _param);

		void *fib = CreateFiber(0, (LPFIBER_START_ROUTINE)&uthread_fiber_start, thr);
		if(!fib)
			return NULL;

		thr->fiber = fib;
		thr->insert(cur);
		return thr;
	}
	
	void uthread::exit()
	{
		uthread_impl *impl = ::netlib::current();
		impl->release();
		impl->suspend();
		throw std::runtime_error("Dead uthread resumed!");
	}

	bool uthread::init()
	{
		return true;
	}

	void uthread::shutdown()
	{
	}
	
	void uthread::enter_thread()
	{
		uthread_impl *impl = new uthread_impl();
		void *f = ConvertThreadToFiber(impl);
		if(!f)
		{
			// TODO: Hold pointer for deletion.
			delete impl;
			return;
		}

		impl->fiber = f;
		impl->acquire();

		enter_thread_common();
	}

	void uthread::exit_thread()
	{
		exit_thread_common();
	}
}
