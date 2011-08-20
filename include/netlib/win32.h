#include "netlib.h"
#include <functional>

#pragma once

namespace netlib
{
	typedef std::function<void(int, int, int)> message_handler_t;

	NETLIB_API bool register_message_handler(int _msg, message_handler_t const& _hdlr);
	NETLIB_API void remove_message_handler(int _msg);
}