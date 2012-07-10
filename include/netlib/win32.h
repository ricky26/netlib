#include "netlib.h"
#include <functional>
#include <Windows.h>

#pragma once

namespace netlib
{
	typedef std::function<void(UINT, LPARAM, WPARAM)> message_handler_t;

	NETLIB_API bool register_message_handler(int _msg, message_handler_t const& _hdlr);
	NETLIB_API void remove_message_handler(int _msg);
}