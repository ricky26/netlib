#include "netlib.h"
#include "atomic.h"
#include <type_traits>

#pragma once

namespace netlib
{
	class NETLIB_API weak_target;
	class NETLIB_API ref_counted
	{
	public:
		typedef size_t ref_count_t;

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

		inline netlib::weak_target *weak_target() const;

	protected:
		virtual void destroy();

		ref_counted();
		ref_count_t mRefCount;
		netlib::weak_target *mWeak;
	};

	class weak_target: public ref_counted
	{
	public:
		friend class ref_counted;

		NETLIB_INLINE ref_counted *target() const { return mTarget; }

	protected:
		inline weak_target(ref_counted *_r): mTarget(_r) {}

		ref_counted *mTarget;
	};

	weak_target *ref_counted::weak_target() const
	{
		if(!mWeak)
		{
			netlib::weak_target *w = new netlib::weak_target((ref_counted*)this);
			w->acquire();
			(netlib::weak_target*&)mWeak = w;
		}

		return mWeak;
	}

	template<typename T=ref_counted>
	class handle
	{
	public:
		typedef T *ptr_t;
		typedef const T * const cptr_t;
		typedef handle<T> handle_t;

		inline handle()
		{
			ptr = nullptr;
		}

		inline handle(cptr_t _t)
		{
			ptr = (ptr_t)_t;
			if(ptr)
				ptr->acquire();
		}

		inline handle(handle_t const& _h)
		{
			ptr = _h.ptr;
			if(ptr)
				ptr->acquire();
		}

		inline handle(handle_t &&_h)
			: ptr(_h.ptr)
		{
			_h.ptr = nullptr;
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
			ptr = nullptr;
			return ret;
		}

		inline void release()
		{
			reset(nullptr);
		}

		template<typename T>
		handle<T> cast() const
		{
			return handle<T>(dynamic_cast<T*>(ptr));
		}

		template<typename T>
		handle<T> cast_static() const
		{
			return handle<T>(static_cast<T*>(ptr));
		}

		inline operator bool() const
		{
			return ptr != nullptr;
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

	template<typename T>
	class weak_handle: public handle<weak_target>
	{
	public:
		typedef handle<weak_target> parent_t;
		typedef weak_handle<T> self_t;
		typedef handle<T> handle_t;
		typedef T *ptr_t;

		inline weak_handle(): parent_t() {}
		inline weak_handle(ptr_t _ptr): parent_t(_ptr->weak_target()) {}

		inline ptr_t get() const
		{
			return parent_t::get()->target();
		}

		inline operator bool() const
		{
			return get() != nullptr;
		}

		inline self_t &operator =(handle_t const& _other)
		{
			reset(_other->weak_target());
			return *this;
		}
	};
}
