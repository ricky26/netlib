#include "netlib.h"

#pragma once

namespace netlib
{
	/**
	 * Call this in any static initializers
	 * that require netlib, to make sure that
	 * it has been setup before the initializer
	 * is run. (This is mostly useful for statically
	 * linked projects.)
	 */
	NETLIB_API void init();

	// Forces a netlib shutdown, you probably don't
	// want to call this, you probably want exit().
	NETLIB_API void shutdown();
};