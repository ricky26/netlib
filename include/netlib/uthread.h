#pragma once

#include "thread_wrapper.h"
#include "ref_counted.h"
#include "thread.h"
#include <list>

namespace std
{
	class exception;
}

namespace netlib
{
	class scheduler;

	class NETLIB_API uthread: public ref_counted
	{
	public:
		friend class uthread_impl;
		friend class scheduler;

		typedef handle<uthread> handle_t;
		typedef std::list<handle_t> list_t;
		typedef std::pair<handle_t, uint64_t> sleep_pair_t;
		typedef std::list<sleep_pair_t> sleep_list_t;
		typedef std::function<void()> run_t;
		
		typedef void (*uthread_start_t)(void *_arg);
		typedef void (*void_fn_t)();

		static handle_t current();
		static handle_t create(uthread_start_t _start, void *_param);

		static inline handle_t create(thread_wrapper0::fn_t _fn)
		{
			void *ptr = new thread_wrapper0(_fn);
			return create(thread_wrapper0::run, ptr);
		}

		template<typename T1>
		static handle_t create(typename thread_wrapper1<T1>::fn_t _fn, T1 _1)
		{
			void *ptr = new thread_wrapper1<T1>(_fn, _1);
			return create(thread_wrapper1<T1>::run, ptr);
		}

		template<typename T1, typename T2>
		static handle_t create(typename thread_wrapper2<T1, T2>::fn_t _fn,
				T1 _1, T2 _2)
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
		static handle_t create(
				typename thread_wrapper4<T1, T2, T3, T4>::fn_t _fn,
				T1 _1, T2 _2, T3 _3, T4 _4)
		{
			typedef thread_wrapper4<T1, T2, T3, T4> wrap_t;
			void *ptr = new wrap_t(_fn, _1, _2, _3, _4);
			return create(wrap_t::run, ptr);
		}
		
		static bool swap(uthread *_other);
		
		void resume();
		
		template<typename T>
		NETLIB_INLINE void raise(T &_exc) { run([_exc]() { throw _exc; }); }

		template<typename T>
		NETLIB_INLINE void timeout_raise(int _ms, T &_exc)
		{
			timeout(_ms, [_exc, this]() { raise(_exc); });
		}

		void run(run_t const& _fn);
		
		NETLIB_INLINE static void timeout(int _ms,
			std::function<void()> const& _fn)
		{
			create([_ms, _fn]() { sleep(_ms); _fn(); });
		}

		static bool schedule();
		static void sleep(int _ms);
		static void suspend();
		static void exit();

		static bool init();
		static void shutdown();

		static void wake_sleepers();
		static bool deadline(uint64_t &);

		// Call on new threads, and dying threads.
		static void enter_thread();
		static void exit_thread();

	private:
		static void enter_thread_common();
		static void exit_thread_common();

		uthread() {};

		bool mSuspended;
		run_t mRun;
	};

	class NETLIB_API channel: public ref_counted
	{
	public:
		typedef void (*read_fn_t)(void *_param, void *_val);

		struct element
		{
			uthread::handle_t thread;
			void *param;
			read_fn_t fn;
		};

		channel();
		~channel();

		void write_ptr(void *_val);
		void read_ptr(read_fn_t _fn, void *_param);

		template<typename T>
		inline void write(T &_var)
		{
			write_ptr(&_var);
		}

		template<typename T>
		inline void read(T &_var)
		{
			read_ptr([&_var](void *_param, void *_val) {
				T *src = reinterpret_cast<T*>(_val);
				_var = *src;
			});
		}

		template<typename T>
		inline T && read()
		{
			T val;
			read(val);
			return std::move(val);
		}

	private:
		int mStatus;
		std::list<element> mValues;
		mutex mLock;
	};
}
