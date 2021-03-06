#include "netlib.h"
#include "ref_counted.h"

#pragma once

namespace netlib
{
	class internal: public ref_counted
	{
	public:
		inline internal() { acquire(); }

		template<typename T>
		static inline T *get(void *&_ptr)
		{
			if(sizeof(T) == sizeof(void*))
				return (T*)(&_ptr);

			return static_cast<T*>(_ptr);
		}

		template<typename T>
		static inline T *create(void *&_ptr)
		{
			if(sizeof(T) == sizeof(void*))
				return new(_ptr) T();

			T *ret = new T();
			_ptr = ret;
			return ret;
		}

		template<typename T>
		static inline void destroy(void *&_ptr)
		{
			if(sizeof(T) == sizeof(void*))
				return static_cast<T*>(
					static_cast<void*>(&_ptr))->~T();

			delete static_cast<T*>(_ptr);
		}
	};

	class NETLIB_API internalized
	{
	public:
		internalized(internalized const& _b);
		internalized(internalized &&);
		~internalized();

		internalized &operator =(internalized const& _b);
		inline bool operator ==(internalized const& _b) const { return mInternal == _b.mInternal; }
		inline bool operator !=(internalized const& _b) const { return mInternal != _b.mInternal; }

	protected:
		inline internalized(): mInternal(nullptr) {}
		internalized(internal *_internal);

		template<typename T>
		inline T *create()
		{
			return internal::create<T>((void*&)mInternal);
		}

		template<typename T>
		inline void destroy()
		{
			internal::destroy<T>(mInternal);
		}

		template<typename T>
		inline T *get() const
		{
			return internal::get<T>((void*&)mInternal);
		}

		inline internal *get() const { return mInternal; }

		void set(internal *_ptr);

	private:
		internal *mInternal;
	};
}
