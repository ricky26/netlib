#include "netlib.h"
#include "atomic.h"

#pragma once

namespace netlib
{
	class NETLIB_API ref_counted
	{
	public:
		typedef uint32_t ref_count_t;

		virtual ~ref_counted();

		inline ref_count_t acquire() { return atomic::increment(&mRefCount); }
		inline ref_count_t release()
		{
			if(mRefCount <= 0)
				return 0;
			
			ref_count_t ret = atomic::decrement(&mRefCount);

			if(ret == 0)
				destroy();

			return ret;
		}

		inline ref_count_t ref_count() { return mRefCount; }

	protected:
		virtual void destroy();

		ref_counted();
		ref_count_t mRefCount;
	};

	template<typename T>
	class handle
	{
	public:
		typedef T *ptr_t;
		typedef handle<T> handle_t;

		inline handle()
		{
			ptr = NULL;
		}

		inline handle(ptr_t _t)
		{
			ptr = _t;
			if(ptr)
				ptr->acquire();
		}

		inline handle(handle_t const& _h)
		{
			ptr = _h.ptr;
			if(ptr)
				ptr->acquire();
		}

		template<typename T>
		inline handle(handle<T> const& _h)
		{
			ptr = _h.get();
			if(ptr)
				ptr->acquire();
		}

		inline ~handle()
		{
			if(ptr)
				ptr->release();
		}

		void reset(ptr_t _p)
		{
			ptr_t old = ptr;
			ptr = _p;

			if(ptr)
				ptr->acquire();

			if(old)
				old->release();
		}

		inline ptr_t get() const
		{
			return ptr;
		}

		inline ptr_t take()
		{
			ptr_t ret = ptr;
			ptr = NULL;
			return ret;
		}

		inline void release()
		{
			reset(NULL);
		}

		template<typename T>
		handle<T> cast() const
		{
			return handle<T>(dynamic_cast<T*>(ptr));
		}

		inline operator bool() const
		{
			return ptr != NULL;
		}

		inline bool operator ==(handle_t const& _other) const
		{
			return ptr == _other.ptr;
		}

		inline bool operator !=(handle_t const& _other) const
		{
			return ptr != _other.ptr;
		}

		inline handle_t &operator =(handle_t const& _other)
		{
			reset(_other.ptr);
			return *this;
		}

		inline ptr_t operator ->() const
		{
			return get();
		}

	private:
		ptr_t ptr;
	};
}
