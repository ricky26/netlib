#include "netlib.h"

#pragma once

namespace netlib
{
	namespace internal
	{
		template<typename T>
		inline T *get(void *&_ptr)
		{
			if(sizeof(T) == sizeof(void*))
				return (T*)(&_ptr);

			return static_cast<T*>(_ptr);
		}

		template<typename T>
		inline T *create(void *&_ptr)
		{
			if(sizeof(T) == sizeof(void*))
				return new(_ptr) T();

			T *ret = new T();
			_ptr = ret;
			return ret;
		}
	}
}