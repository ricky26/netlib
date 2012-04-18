#include "netlib.h"

#pragma once

namespace netlib
{
	template<typename T>
	struct lock_holder
	{
		inline lock_holder(...)
		{
			static_assert(false, "Cannot lock T");
		}
	};

	template<typename T>
	struct try_lock_holder
	{
		inline try_lock_holder(...)
		{
			static_assert(false, "Cannot lock T");
		}
	};

	template<typename T>
	struct lock_holder_base
	{
		T &locked;

		inline lock_holder_base(T &_l): locked(_l)
		{
			locked.lock();
		}

		inline ~lock_holder_base() { locked.unlock(); };

		inline operator bool() const { return true; }
	};

	template<typename T>
	struct try_lock_holder_base
	{
		T &locked;
		bool value;

		inline try_lock_holder_base(T &_l): locked(_l)
		{
			value = locked.try_lock();
		}

		inline ~try_lock_holder_base() { if(value) locked.unlock(); };

		inline operator bool() const { return value; }
	};

	template<typename T>
	inline lock_holder<T> lock(T &_lock) { return lock_holder<T>(_lock); }
	template<typename T>
	inline try_lock_holder<T> try_lock(T &_lock) { return try_lock_holder(_lock); }
}
