#include "netlib/base64.h"
#include <Windows.h>
#include <WinCrypt.h>

namespace netlib
{
	base64::base64()
	{
	}

	base64::~base64()
	{
	}
		
	std::string base64::encode(std::string const& _str)
	{
		std::string ret;
		ret.resize((_str.length()*3)/2, 0);

		DWORD outSize = (DWORD)ret.size();
		if(CryptBinaryToStringA((const BYTE*)_str.data(), (DWORD)_str.size(),
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, (char*)ret.data(), &outSize) != TRUE)
			return std::string();

		if(ret[outSize-2] == '\r')
			ret.resize(outSize-2);
		else
			ret.resize(outSize);

		return ret;
	}

	std::string base64::decode(std::string const& _str)
	{
		std::string ret;
		ret.resize((_str.length()*2)/3, 0);

		DWORD outSize = (DWORD)ret.size();
		if(CryptStringToBinaryA(_str.data(), (DWORD)_str.size(),
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, (BYTE*)ret.data(), &outSize, NULL, NULL) != TRUE)
			return std::string();

		return ret;
	}
}