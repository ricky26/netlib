#include "netlib.h"

#pragma once

namespace netlib
{
	class NETLIB_API ref_counted
	{
	public:
		typedef uint32_t ref_count_t;

		virtual ~ref_counted();
		virtual void destroy();

		inline ref_count_t acquire() { return mRefCount++; }
		inline ref_count_t release()
		{
			if(mRefCount <= 0)
				return 0;
			
			ref_count_t ret = mRefCount--;

			if(mRefCount == 0)
				destroy();

			return ret;
		}

		inline ref_count_t ref_count() { return mRefCount; }

	protected:
		ref_counted();
		ref_count_t mRefCount;
	};

	template<typename T>
	class handle
	{
	public:
		typedef T *ptr_t;
		typedef handle<T> handle_t;

		handle()
		{
			ptr = NULL;
		}

		handle(ptr_t _t)
		{
			ptr = _t;
			if(ptr)
				ptr->acquire();
		}

		handle(handle_t const& _h)
		{
			ptr = _h.ptr;
			if(ptr)
				ptr->acquire();
		}

		~handle()
		{
			if(ptr)
				ptr->release();
		}

		void reset(ptr_t _p)
		{
			if(ptr == _p)
				return;

			if(ptr)
				ptr->release();

			ptr = _p;

			if(ptr)
				ptr->acquire();
		}

		ptr_t get() const
		{
			return ptr;
		}

		ptr_t take()
		{
			ptr_t ret = ptr;
			ptr = NULL;
			return ret;
		}

		void release()
		{
			reset(NULL);
		}

		handle_t &operator =(handle_t const& _src)
		{
			reset(_src.ptr);
			return *this;
		}

		ptr_t operator ->() const
		{
			return get();
		}

	private:
		ptr_t ptr;
	};
}
