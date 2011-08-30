#include "netlib.h"

#pragma once

namespace netlib
{
	class NETLIB_API data
	{
	public:
		NETLIB_INLINE data() : mData(NULL), mSize(0) {}
		NETLIB_INLINE data(void *_ptr, size_t _sz): mData(_ptr), mSize(_sz) {}
		
		void set(data &_d);
		void set(void *_ptr, size_t _sz);
		NETLIB_INLINE void *take();

		NETLIB_INLINE void *ptr() const { return mData; }
		NETLIB_INLINE size_t size() const { return mSize; }

		template<typename T>
		NETLIB_INLINE T *ptr() const { return static_cast<T*>(mData); }

		NETLIB_INLINE data &operator =(data &_d) { set(_d); }

	private:
		void *mData;
		size_t mSize;
	};
}
