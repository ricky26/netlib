#include "crypto.h"

#pragma once

namespace netlib
{
	class NETLIB_API base64: public crypto::encoding
	{
	public:
		base64();
		~base64();
		
		virtual std::string encode(std::string const& _str);
		virtual std::string decode(std::string const& _str);
	};
}