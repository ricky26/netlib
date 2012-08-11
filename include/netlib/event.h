#include "netlib.h"
#include <functional>
#include <unordered_set>

#pragma once

namespace netlib
{
	class NETLIB_API event
	{
	public:
		event(void *_sender=nullptr);

		inline void *sender() const { return mSender; }

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

		inline void clear() { handlers.clear(); }
		inline return_t add_handler(handler_t const& _h) { handlers.push_back(_h); return handlers.end()--; }
		inline void remove_handler(return_t const& _r) { handlers.erase(_r); }

		bool notify(T const& _evt)
		{
			for(auto it = handlers.begin(); it != handlers.end(); it++)
			{
				const handler_t &h(*it);

				if(!h)
				{
					auto it2 = it;
					it2++;

					handlers.erase(it);
					
					if(it2 == handlers.end())
						break;
				}

				if(h(_evt))
					return true;
			}

			return false;
		}
		
		inline bool operator ()(T const& _evt) { return notify(_evt); }
		
		inline return_t operator +=(handler_t const& _h) { return add_handler(_h); }
		inline void operator -=(return_t const& _h) { remove_handler(_h); }

	private:
		list_t handlers;
	};
}
