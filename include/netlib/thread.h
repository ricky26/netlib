#include "thread_wrapper.h"
#include "ref_counted.h"

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
		bool suspend();
		bool resume();

		int protection() const;
		static int protect();
		static int unprotect();
		
		static handle_t current();

		static handle_t create(thread_fn_t _fn, void *_arg);
		static handle_t create(void_fn_t _fn);

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

		static bool init();
		static void shutdown();

	private:
		thread();
	};
}
