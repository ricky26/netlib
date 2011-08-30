#include "netlib.h"
#include <functional>
#include <unordered_set>

#pragma once

namespace netlib
{
	class NETLIB_API event
	{
	public:
		event(void *_sender=NULL);

		NETLIB_INLINE void *sender() const { return mSender; }

	private:
		void *mSender;
	};

	template<typename T=event>
	class delegate
	{
	public:
		typedef std::function<bool (T const&)> handler_t;
		typedef std::list<handler_t> list_t;
		typedef typename list_t::iterator return_t;

		NETLIB_INLINE void clear() { handlers.clear(); }
		NETLIB_INLINE return_t add_handler(handler_t const& _h) { handlers.push_back(_h); return handlers.end()--; }
		NETLIB_INLINE void remove_handler(return_t const& _r) { handlers.erase(_r); }

		NETLIB_INLINE bool notify(T const& _evt)
		{
			for(auto it = handlers.begin(); it != handlers.end(); it++)
			{
				if((*it)(_evt))
					return true;
			}

			return false;
		}
		
		NETLIB_INLINE bool operator ()(T const& _evt) { return notify(_evt); }
		
		NETLIB_INLINE return_t operator +=(handler_t const& _h) { return add_handler(_h); }
		NETLIB_INLINE void operator -=(return_t const& _h) { remove_handler(_h); }

	private:
		list_t handlers;
	};
}
