#include "netlib.h"

#pragma once

namespace netlib
{
	template<typename T>
	struct owned_ptr_delete
	{
		static void destroy(T *_ptr)
		{
			delete _ptr;
		}
	};

	template<>
	struct owned_ptr_delete<void>
	{
		static void destroy(void *_ptr)
		{
			delete _ptr;
		}
	};

	template<typename T=void>
	class owned_ptr
	{
	public:
		typedef owned_ptr<T> self_t;
		typedef T *ptr_t;

		NETLIB_INLINE owned_ptr() : ptr(NULL) {}

		NETLIB_INLINE owned_ptr(ptr_t const& _ptr)
			: ptr(_ptr) {}

		NETLIB_INLINE owned_ptr(self_t &_ptr)
			: ptr(_ptr.ptr)
		{
			_ptr.ptr = NULL;
		}

		template<typename Z>
		NETLIB_INLINE owned_ptr(owned_ptr<Z> &_ptr)
			: ptr(static_cast<ptr_t>(_ptr.ptr))
		{
			_ptr.ptr = NULL;
		}

		NETLIB_INLINE ~owned_ptr()
		{
			owned_ptr_delete<T>::destroy(ptr);
		}

		NETLIB_INLINE void set(ptr_t const& _ptr)
		{
			if(ptr)
				owned_ptr_delete<T>::destroy(ptr);
			ptr = _ptr;
		}

		NETLIB_INLINE ptr_t get() const { return ptr; }

		NETLIB_INLINE ptr_t take()
		{
			ptr_t ret = ptr;
			ptr = nullptr;
			return ret;
		}

		NETLIB_INLINE self_t &operator =(ptr_t const& _ptr)
		{
			set(_ptr);
			return *this;
		}

	private:
		ptr_t ptr;
	};
}