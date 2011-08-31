#include "netlib.h"
#include "owned_ptr.h"

#pragma once

namespace netlib
{
	class data
	{
	public:
		NETLIB_INLINE data() : mSize(0) {}
		NETLIB_INLINE data(void *_ptr, size_t _sz): mData(_ptr), mSize(_sz) {}
		
		NETLIB_INLINE void set(data &_d) { set(_d.ptr(), _d.size()); }
		NETLIB_INLINE void set(void *_ptr, size_t _sz) { mData = _ptr; mSize = _sz; }
		NETLIB_INLINE void *take() { return mData.take(); }

		NETLIB_INLINE void *ptr() const { return mData.get(); }
		NETLIB_INLINE size_t size() const { return mSize; }

		template<typename T>
		NETLIB_INLINE T *ptr() const { return static_cast<T*>(mData.get()); }

		NETLIB_INLINE data &operator =(data &_d) { set(_d); return *this; }

	private:
		owned_ptr<> mData;
		size_t mSize;
	};
}
