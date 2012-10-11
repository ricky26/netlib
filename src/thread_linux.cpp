#include "netlib/thread.h"
#include "netlib/uthread.h"
#include <time.h>
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
		thread_impl(thread_fn_t _fn=nullptr, void *_arg=nullptr,
			pthread_t _handle=0)
			: handle(_handle), function(_fn), argument(_arg)
		{
			acquire();
		}

		static void sigsegv_handler(int)
		{
			throw std::exception();
		}

		static void setup()
		{
			signal(SIGSEGV, sigsegv_handler);

			if(!current)
				current = new thread_impl(nullptr, nullptr, pthread_self());

			current->acquire();

			uthread::enter_thread();
		}

		static void cleanup()
		{
			uthread::exit_thread();

			if(current)
				current->release();
		}

		static __thread thread_impl *current;
		thread_fn_t function;
		void *argument;
		pthread_t handle;
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
	
	void thread::exit()
	{
	}

	void thread::schedule()
	{
	}

	void thread::sleep(int _ms)
	{
		//msleep(_ms);
	}

	thread::handle_t thread::current()
	{
		return thread_impl::current;
	}

	static void *thread_fn(void *_param)
	{
		thread_impl *ths = (thread_impl*)_param;
		ths->current = ths;
		
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

		return nullptr;
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
