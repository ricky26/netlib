#include "netlib.h"
#include "ref_counted.h"
#include <functional>
#include <cstdlib>

#pragma once

namespace netlib
{
	class data: public ref_counted
	{
	public:
		typedef handle<data> handle_t;
		typedef std::function<void(void*)> delete_fn_t;

		NETLIB_INLINE data() : mData(NULL), mSize(0) {}
		NETLIB_INLINE data(void *_ptr, size_t _sz, delete_fn_t _fn=&std::free)
			: mData(_ptr), mSize(_sz), mFn(_fn) {}

		NETLIB_INLINE ~data() { if(mData) mFn(mData); }
		
		NETLIB_INLINE void set(handle_t const& _d) { set(_d->ptr(), _d->size()); }
		NETLIB_INLINE void set(void *_ptr, size_t _sz) { mData = _ptr; mSize = _sz; }

		NETLIB_INLINE void *ptr() const { return mData; }
		NETLIB_INLINE size_t size() const { return mSize; }

		template<typename T>
		NETLIB_INLINE T *ptr() const { return static_cast<T*>(mData.get()); }

		static void no_free(void*) {};

	private:
		void *mData;
		size_t mSize;
		delete_fn_t mFn;
	};
}
