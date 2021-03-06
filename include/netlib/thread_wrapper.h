#include "netlib.h"
#include <functional>

#pragma once

namespace netlib
{
	struct thread_wrapper0
	{
		typedef thread_wrapper0 self_t;
		typedef std::function<void()> fn_t;

		fn_t fn;

		inline thread_wrapper0(fn_t _fn): fn(_fn)
		{
		}

		static inline void run(void *_p)
		{
			self_t *self = static_cast<self_t*>(_p);
			self->fn();
			delete self;
		}
	};

	template<typename T1>
	struct thread_wrapper1
	{
		typedef thread_wrapper1<T1> self_t;
		typedef std::function<void(T1)> fn_t;

		fn_t fn;
		T1 a1;

		inline thread_wrapper1(fn_t _fn, T1 _1): fn(_fn), a1(_1)
		{
		}

		static inline void run(void *_p)
		{
			self_t *self = static_cast<self_t*>(_p);
			self->fn(self->a1);
			delete self;
		}
	};

	template<typename T1, typename T2>
	struct thread_wrapper2
	{
		typedef thread_wrapper2<T1, T2> self_t;
		typedef std::function<void(T1, T2)> fn_t;

		fn_t fn;
		T1 a1;
		T2 a2;

		inline thread_wrapper2(fn_t _fn, T1 _1, T2 _2)
			: fn(_fn), a1(_1), a2(_2)
		{
		}

		static inline void run(void *_p)
		{
			self_t *self = static_cast<self_t*>(_p);
			self->fn(self->a1, self->a2);
			delete self;
		}
	};

	template<typename T1, typename T2, typename T3>
	struct thread_wrapper3
	{
		typedef thread_wrapper3<T1, T2, T3> self_t;
		typedef std::function<void(T1, T2, T3)> fn_t;

		fn_t fn;
		T1 a1;
		T2 a2;
		T3 a3;

		inline thread_wrapper3(fn_t _fn, T1 _1, T2 _2, T3 _3)
			: fn(_fn), a1(_1), a2(_2), a3(_3)
		{
		}

		static inline void run(void *_p)
		{
			self_t *self = static_cast<self_t*>(_p);
			self->fn(self->a1, self->a2, self->a3);
			delete self;
		}
	};

	template<typename T1, typename T2, typename T3, typename T4>
	struct thread_wrapper4
	{
		typedef thread_wrapper4<T1, T2, T3, T4> self_t;
		typedef std::function<void(T1, T2, T3, T4)> fn_t;

		fn_t fn;
		T1 a1;
		T2 a2;
		T3 a3;
		T4 a4;

		inline thread_wrapper4(fn_t _fn, T1 _1, T2 _2, T3 _3, T4 _4)
			: fn(_fn), a1(_1), a2(_2), a3(_3), a4(_4)
		{
		}

		static inline void run(void *_p)
		{
			self_t *self = static_cast<self_t*>(_p);
			self->fn(self->a1, self->a2, self->a3, self->a4);
			delete self;
		}
	};
}
