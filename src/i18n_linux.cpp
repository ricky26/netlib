#include "netlib/i18n.h"
#include <stdexcept>

namespace netlib
{
	namespace i18n
	{
		NETLIB_API std::wstring utf8_to_utf16(std::string const& _text)
		{
			throw std::runtime_error("not implemented");
		}

		NETLIB_API std::string utf16_to_utf8(std::wstring const& _text)
		{
			throw std::runtime_error("not implemented");
		}
	}
}
