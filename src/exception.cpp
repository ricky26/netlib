#include "netlib/exception.h"

namespace netlib
{
	//
	// quit_exception
	//

	quit_exception::quit_exception(int _code)
		: mValue(_code), mMessage("Quit with code " + _code)
	{
	}
}
