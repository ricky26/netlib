#include "netlib/thread.h"
#include "netlib/uthread.h"
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <exception>
#include <stdexcept>
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
			throw std::runtime_error("segmentation fault");
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
		uint64_t dl = netlib::time() + _ms * 1000;
		uint64_t t;

		while(t < dl)
		{
			uint64_t d = dl-t;
			if(d > 1000000)
				sleep(d/1000000);
			else
				usleep(d);
		}
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

	//
	// thread_condition
	//

	static inline pthread_cond_t *cond_ptr(void *&_base)
	{
		if(sizeof(pthread_cond_t) <= sizeof(void*))
			return (pthread_cond_t*)&_base;
		
		return (pthread_cond_t*)_base;
	}

	thread_condition::thread_condition()
	{
		if(sizeof(pthread_cond_t) > sizeof(void*))
			mInternal = new pthread_cond_t;

		pthread_cond_init(cond_ptr(mInternal), NULL);
	}

	thread_condition::~thread_condition()
	{
		pthread_cond_destroy(cond_ptr(mInternal));
		
		if(sizeof(pthread_cond_t) > sizeof(void*))
			delete (pthread_cond_t*)mInternal;
	}

	void thread_condition::signal()
	{
		pthread_cond_signal(cond_ptr(mInternal));
	}

	void thread_condition::broadcast()
	{
		pthread_cond_broadcast(cond_ptr(mInternal));
	}

	//
	// Mutex
	//
 
	static inline pthread_mutex_t *mutex_ptr(void *&_base)
	{
		if(sizeof(pthread_mutex_t)<=sizeof(void*))
			return (pthread_mutex_t*)&_base;
		
		return (pthread_mutex_t*)_base;
	}

	mutex::mutex()
	{
		if(sizeof(pthread_mutex_t)>sizeof(void*))
			mInternal = new pthread_mutex_t;

		pthread_mutex_init(mutex_ptr(mInternal), NULL);
	}

	mutex::~mutex()
	{
		pthread_mutex_destroy(mutex_ptr(mInternal));

		if(sizeof(pthread_mutex_t)>sizeof(void*))
			delete (pthread_mutex_t*)mInternal;
	}

	bool mutex::try_lock()
	{
		return pthread_mutex_trylock(mutex_ptr(mInternal)) == 0;
	}

	void mutex::lock()
	{
		pthread_mutex_lock(mutex_ptr(mInternal));
	}

	void mutex::unlock()
	{
		pthread_mutex_unlock(mutex_ptr(mInternal));
	}

	void mutex::wait(thread_condition &_con)
	{
		pthread_cond_wait(cond_ptr(_con.mInternal), mutex_ptr(mInternal));
	}

	//
	// rw_lock
	//

	static inline pthread_rwlock_t *rwlock_ptr(void *&_base)
	{
		if(sizeof(pthread_rwlock_t)<=sizeof(void*))
			return (pthread_rwlock_t*)&_base;
		
		return (pthread_rwlock_t*)_base;
	}

	rw_lock::rw_lock()
	{
		if(sizeof(pthread_rwlock_t) > sizeof(void*))
			mInternal = new pthread_rwlock_t;

		pthread_rwlock_init(rwlock_ptr(mInternal), NULL);
	}

	rw_lock::~rw_lock()
	{
		pthread_rwlock_destroy(rwlock_ptr(mInternal));
	}

	bool rw_lock::try_lock_read()
	{
		return !pthread_rwlock_tryrdlock(rwlock_ptr(mInternal));
	}

	void rw_lock::lock_read()
	{
		pthread_rwlock_rdlock(rwlock_ptr(mInternal));
	}

	void rw_lock::unlock_read()
	{
		pthread_rwlock_unlock(rwlock_ptr(mInternal));
	}

	bool rw_lock::try_lock_write()
	{
		return !pthread_rwlock_trywrlock(rwlock_ptr(mInternal));
	}

	void rw_lock::lock_write()
	{
		pthread_rwlock_wrlock(rwlock_ptr(mInternal));
	}

	void rw_lock::unlock_write()
	{
		pthread_rwlock_unlock(rwlock_ptr(mInternal));
	}
}
