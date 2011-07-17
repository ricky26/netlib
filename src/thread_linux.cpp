#include "netlib/thread.h"
#include "netlib/uthread.h"
#include <pthread.h>
#include <signal.h>
#include <exception>
#include <iostream>

namespace netlib
{
	//
	// thread_impl
	//

	class thread_impl: public thread
	{
	public:
		thread_impl(thread_fn_t _fn, void *_arg)
			: handle(0), function(_fn), argument(_arg)
			, protection(0)
		{
		}

		thread_impl(pthread_t _h)
			: handle(_h), function(NULL), argument(NULL)
			, protection(0)
		{
		}

		static void sigsegv_handler(int)
		{
			throw std::exception();
		}

		static void setup()
		{
			signal(SIGSEGV, sigsegv_handler);

			if(!current)
				current = new thread_impl(pthread_self());
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

		static __thread thread_impl *current;
		thread_fn_t function;
		void *argument;
		pthread_t handle;
		int protection;
	};

	thread_impl __thread *thread_impl::current = NULL;

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

	static void *thread_fn(void *_param)
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

		return (void*)ret;
	}

	thread::handle_t thread::create(thread_fn_t _fn, void *_arg)
	{
		thread_impl *ret = new thread_impl(_fn, _arg);
		pthread_t handle;

		int res = pthread_create(&handle, NULL, thread_fn, ret);
		if(res < 0)
			return NULL;

		ret->handle = handle;
		return ret;
	}

	static void create_void(void *_arg)
	{
		((thread::void_fn_t)_arg)();
	}

	thread::handle_t thread::create(void_fn_t _fn)
	{
		return create(create_void, (void*)_fn);
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
