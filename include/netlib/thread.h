#include "thread_wrapper.h"
#include "ref_counted.h"
#include "lock.h"

#pragma once

namespace netlib
{
	class NETLIB_API thread: public ref_counted
	{
	public:
		friend class thread_impl;

		typedef handle<thread> handle_t;

		typedef void (*void_fn_t)();
		typedef void (*thread_fn_t)(void *_arg);

		virtual ~thread();

		bool join();
		static void exit();
		static void schedule();
		static void sleep(int _ms);
		
		static handle_t current();
		static handle_t create(thread_fn_t _fn, void *_arg);

		static inline handle_t create(thread_wrapper0::fn_t _fn)
		{
			void *ptr = new thread_wrapper0(_fn);
			return create(thread_wrapper0::run, ptr);
		}

		template<typename T1>
		static inline handle_t create(typename thread_wrapper1<T1>::fn_t _fn, T1 _1)
		{
			void *ptr = new thread_wrapper1<T1>(_fn, _1);
			return create(thread_wrapper1<T1>::run, ptr);
		}

		template<typename T1, typename T2>
		static inline handle_t create(typename thread_wrapper2<T1, T2>::fn_t _fn, T1 _1, T2 _2)
		{
			void *ptr = new thread_wrapper2<T1, T2>(_fn, _1, _2);
			return create(thread_wrapper2<T1, T2>::run, ptr);
		}

		template<typename T1, typename T2, typename T3>
		static inline handle_t create(typename thread_wrapper3<T1, T2, T3>::fn_t _fn,
				T1 _1, T2 _2, T3 _3)
		{
			typedef thread_wrapper3<T1, T2, T3> wrap_t;
			void *ptr = new wrap_t(_fn, _1, _2, _3);
			return create(wrap_t::run, ptr);
		}

		template<typename T1, typename T2, typename T3, typename T4>
		static inline handle_t create(typename thread_wrapper4<T1, T2, T3, T4>::fn_t _fn,
				T1 _1, T2 _2, T3 _3, T4 _4)
		{
			typedef thread_wrapper4<T1, T2, T3, T4> wrap_t;
			void *ptr = new wrap_t(_fn, _1, _2, _3, _4);
			return create(wrap_t::run, ptr);
		}

		static bool init();
		static void shutdown();

	private:
		thread();
	};

	class NETLIB_API thread_condition
	{
	public:
		friend class mutex;

		thread_condition();
		virtual ~thread_condition();

	private:
		void *mInternal;
	};
	
	class NETLIB_API mutex
	{
	public:
		mutex();
		virtual ~mutex();

		bool try_lock();
		void lock();
		void unlock();

		void wait(thread_condition&);

	private:
		void *mInternal;
	};

	class NETLIB_API rw_lock
	{
	public:
		rw_lock();
		virtual ~rw_lock();

		bool try_lock_read();
		void lock_read();
		void unlock_read();

		bool try_lock_write();
		void lock_write();
		void unlock_write();

		class read_lock
		{
		public:
			rw_lock &lck;

			inline read_lock(rw_lock &_lock): lck(_lock) {}
			
			inline bool try_lock() { return lck.try_lock_read(); }
			inline void lock() { return lck.lock_read(); }
			inline void unlock() { return lck.unlock_read(); }
		};
		inline read_lock read() { return read_lock(*this); }

		class write_lock
		{
		public:
			rw_lock &lck;

			inline write_lock(rw_lock &_lock): lck(_lock) {}
			
			inline bool try_lock() { return lck.try_lock_write(); }
			inline void lock() { return lck.lock_write(); }
			inline void unlock() { return lck.unlock_write(); }
		};
		inline write_lock write() { return write_lock(*this); }

	private:
		void *mInternal;
	};
	
	template<> struct lock_holder<mutex>: lock_holder_base<mutex>
		{ inline lock_holder(mutex &_m): lock_holder_base(_m) {}};
	template<> struct lock_holder<rw_lock::read_lock>: lock_holder_base<rw_lock::read_lock>
		{ inline lock_holder(rw_lock::read_lock &_m): lock_holder_base(_m) {}};
	template<> struct lock_holder<rw_lock::write_lock>: lock_holder_base<rw_lock::write_lock>
		{ inline lock_holder(rw_lock::write_lock &_m): lock_holder_base(_m) {}};
}
