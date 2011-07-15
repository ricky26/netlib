#include "netlib.h"
#include <string>

namespace netlib
{
	NETLIB_API std::string lowercase(std::string const& _str);
	NETLIB_API std::string uppercase(std::string const& _str);

	NETLIB_API bool case_insensitive_compare(std::string const& _a, std::string const& _b);
}
