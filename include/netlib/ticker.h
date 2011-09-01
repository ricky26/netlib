#include "netlib.h"
#include <functional>
#include <list>

#pragma once

namespace netlib
{
	typedef std::function<bool()> ticker_t;
	typedef std::list<ticker_t> ticker_list_t;
	typedef ticker_list_t::iterator ticker_handle_t;
	
	NETLIB_API ticker_handle_t add_ticker(ticker_t const& _t);
	NETLIB_API void remove_ticker(ticker_handle_t const& _t);
	NETLIB_API void execute_tickers();

	typedef std::function<bool()> atstart_t;
	typedef std::list<atstart_t> atstart_list_t;
	NETLIB_API void atstart(atstart_t const& _as);
	NETLIB_API bool execute_atstart();

	typedef std::function<void()> atexit_t;
	typedef std::list<atexit_t> atexit_list_t;
	NETLIB_API void atexit(atexit_t const& _ae);
	NETLIB_API void execute_atexit();
}
