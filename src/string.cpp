#include "netlib/string.h"
#include <string.h>
#include <cctype>

#ifdef _WIN32
#define strncasecmp strnicmp
#endif

namespace netlib
{
	NETLIB_API std::string lowercase(std::string const& _str)
	{
		std::string ret;
		ret.reserve(_str.length());

		for(size_t i = 0; i < _str.length(); i++)
			ret.push_back(std::tolower(_str[i]));

		return ret;
	}

	NETLIB_API std::string uppercase(std::string const& _str)
	{
		std::string ret;
		ret.reserve(_str.length());

		for(size_t i = 0; i < _str.length(); i++)
			ret.push_back(std::toupper(_str[i]));

		return ret;
	}

	NETLIB_API bool case_insensitive_compare(std::string const& _a, std::string const& _b)
	{
		if(_a.size() != _b.size())
			return false;

		return !strncasecmp(_a.data(), _b.data(), _a.size());
	}
}
