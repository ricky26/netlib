#include "netlib/ticker.h"
#include "netlib/thread.h"
#include <algorithm>

namespace netlib
{
	static ticker_list_t gTickers;
	static mutex gTickerCS;
	
	static atstart_list_t gAtStart;
	static mutex gAtStartCS;

	static atexit_list_t gAtExit;
	static mutex gAtExitCS;
	
	NETLIB_API ticker_handle_t add_ticker(ticker_t const& _t)
	{
		gTickerCS.lock();
		gTickers.push_back(_t);
		ticker_handle_t ret = gTickers.end();
		ret--;
		gTickerCS.unlock();
		return ret;
	}

	NETLIB_API void remove_ticker(ticker_handle_t const& _t)
	{
		gTickerCS.lock();
		gTickers.erase(_t);
		gTickerCS.unlock();
	}

	NETLIB_API void execute_tickers()
	{
		gTickerCS.lock();
		std::remove_if(gTickers.begin(), gTickers.end(), [](ticker_t const& _t) -> bool { return !_t(); });
		gTickerCS.unlock();
	}

	NETLIB_API void atstart(atstart_t const& _as)
	{
		gAtStartCS.lock();
		gAtStart.push_back(_as);
		gAtStartCS.unlock();
	}

	NETLIB_API bool execute_atstart()
	{
		gAtStartCS.lock();
		auto it = std::find_if(gAtStart.begin(), gAtStart.end(), [](atstart_t const& _t) -> bool { return !_t(); });
		gAtStartCS.unlock();

		return it == gAtStart.end();
	}

	NETLIB_API void atexit(atexit_t const& _ae)
	{
		gAtExitCS.lock();
		gAtExit.push_back(_ae);
		gAtExitCS.unlock();
	}

	NETLIB_API void execute_atexit()
	{
		gAtExitCS.lock();
		std::for_each(gAtExit.begin(), gAtExit.end(), [](atexit_t const& _t) { _t(); });
		gAtExitCS.unlock();
	}
}
