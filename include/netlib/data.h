#include "netlib.h"
#include "ref_counted.h"
#include <functional>
#include <cstdlib>

#pragma once

namespace netlib
{
	class NETLIB_API data: public ref_counted
	{
	public:
		typedef handle<data> handle_t;
		typedef std::function<void(void*)> delete_fn_t;

		NETLIB_INLINE data() : mData(nullptr), mSize(0), mFn(nullptr) {}
		NETLIB_INLINE data(data &&_d): mData(_d.mData), mSize(_d.mSize), mFn(_d.mFn)
		{
			_d.clear_data();
		}

		NETLIB_INLINE data(handle_t const& _h) : mData(_h->mData), mSize(_h->mSize),
			mFn(_h->mFn)
		{
			_h->clear_data();
		}

		NETLIB_INLINE data(void *_ptr, size_t _sz, delete_fn_t _fn=&std::free)
			: mData(_ptr), mSize(_sz), mFn(_fn) {}

		NETLIB_INLINE ~data() { release_data(); }

		NETLIB_INLINE bool valid() const { return mData != nullptr; }
		
		NETLIB_INLINE void set(data &&_d)
		{
			set(_d.ptr(), _d.size(), _d.delete_function());
			_d.clear_data();
		}

		NETLIB_INLINE void set(handle_t const& _d)
		{
			set(_d->ptr(), _d->size(), _d->delete_function());
			_d->clear_data();
		}

		NETLIB_INLINE void set(void *_ptr, size_t _sz, delete_fn_t _fn=&std::free)
			{ release_data(); mData = _ptr; mSize = _sz; mFn = _fn; }
		NETLIB_INLINE void set_static(void *_ptr, size_t _sz) { set(_ptr, _sz, &data::no_free); }

		NETLIB_INLINE void *ptr() const { return mData; }
		NETLIB_INLINE size_t size() const { return mSize; }
		NETLIB_INLINE delete_fn_t delete_function() const { return mFn; }

		template<typename T>
		NETLIB_INLINE T *ptr() const { return static_cast<T*>(mData.get()); }

		inline data &operator =(data &&_b) { set(std::forward<data&&>(_b)); return *this; }
		
		static void no_free(void*) {};
		static inline data static_data(void *_data, size_t _sz)
		{
			return data(_data, _sz, &data::no_free);
		}

		static handle_t copy(void *_data, size_t _sz);

	protected:
		data(data const&);

		inline void clear_data()
		{
			mData = nullptr;
			mSize = 0;
		}

		inline void release_data()
		{
			if(mData)
			{
				mFn(mData);
				clear_data();
			}
		}

	private:
		void *mData;
		size_t mSize;
		delete_fn_t mFn;
	};
}
