#pragma once

#include "thread_wrapper.h"
#include "ref_counted.h"

namespace std
{
	class exception;
}

namespace netlib
{
	class scheduler;
	class thread;

	class NETLIB_API uthread: public ref_counted
	{
	public:
		friend class uthread_impl;

		typedef handle<uthread> handle_t;

		virtual ~uthread();
		virtual void destroy();
		
		typedef void (*uthread_start_t)(void *_arg);
		typedef void (*void_fn_t)();

		static handle_t current();
		static handle_t create(void_fn_t _start);
		static handle_t create(uthread_start_t _start, void *_param);

		template<typename T1>
		static handle_t create(typename thread_wrapper1<T1>::fn_t _fn, T1 _1)
		{
			void *ptr = new thread_wrapper1<T1>(_fn, _1);
			return create(thread_wrapper1<T1>::run, ptr);
		}

		template<typename T1, typename T2>
		static handle_t create(typename thread_wrapper2<T1, T2>::fn_t _fn, T1 _1, T2 _2)
		{
			void *ptr = new thread_wrapper2<T1, T2>(_fn, _1, _2);
			return create(thread_wrapper2<T1, T2>::run, ptr);
		}

		template<typename T1, typename T2, typename T3>
		static handle_t create(typename thread_wrapper3<T1, T2, T3>::fn_t _fn,
				T1 _1, T2 _2, T3 _3)
		{
			typedef thread_wrapper3<T1, T2, T3> wrap_t;
			void *ptr = new wrap_t(_fn, _1, _2, _3);
			return create(wrap_t::run, ptr);
		}

		template<typename T1, typename T2, typename T3, typename T4>
		static handle_t create(typename thread_wrapper4<T1, T2, T3, T4>::fn_t _fn,
				T1 _1, T2 _2, T3 _3, T4 _4)
		{
			typedef thread_wrapper4<T1, T2, T3, T4> wrap_t;
			void *ptr = new wrap_t(_fn, _1, _2, _3, _4);
			return create(wrap_t::run, ptr);
		}
		
		static bool swap(uthread *_other);
		static inline bool swap(handle_t _h) { return swap(_h.get()); }
		
		void resume();

		template<typename T>
		inline void raise(T &_exc)
		{
			redirect_exceptions(this);

			try
			{
				throw _exc;
			}
			catch(...)
			{}

			redirect_exceptions(NULL);
		}

		static void schedule();
		void schedule(scheduler*);
		netlib::scheduler *scheduler() const;

		static void suspend();
		static void exit();

		int protection() const;
		static int protect();
		static int unprotect();

		static void redirect_exceptions(handle_t _h);

		static bool init();
		static void shutdown();
		static void enable_uthread(); // Enables uthreads on the current thread.

	private:
		uthread();
	};

	class NETLIB_API scheduler
	{
	public:
		friend class uthread;
		friend class uthread_impl;

		scheduler();
		virtual ~scheduler();

		void insert(uthread *_thr);
		void schedule();

		handle<thread> create_worker_thread();
		void create_worker_threads(int _icnt);

	protected:
		uthread *mFirst;
		uthread *mLast;
		void *mInternal;
	};
}
