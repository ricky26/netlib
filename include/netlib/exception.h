#include "internal.h"

#pragma once

namespace netlib
{
	class NETLIB_API stack_trace: public internalized
	{
	public:
		stack_trace();

		bool valid() const;
		bool capture(size_t _offset=0);
	};

	//
	// Standard Exceptions
	//

	class NETLIB_API quit_exception: public std::exception
	{
	public:
		quit_exception(int _code);

		int value() const { return mValue; }
		virtual const char *what() const override { return mMessage.c_str(); }

	protected:
		int mValue;
		std::string mMessage;
	};
};
