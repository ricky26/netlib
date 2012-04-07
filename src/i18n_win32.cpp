#include "netlib/i18n.h"
#include <Windows.h>

namespace netlib
{
	namespace i18n
	{
		NETLIB_API std::wstring utf8_to_utf16(std::string const& _text)
		{
			std::wstring ret;
			ret.resize(_text.size());

			int rval = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				_text.data(), (int)_text.size(), (wchar_t*)ret.data(), (int)ret.capacity());

			if(rval == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				rval = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
					_text.data(), (int)_text.size(), nullptr, 0);
				if(rval)
				{
					ret.resize(rval+1);
					rval = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
						_text.data(), (int)_text.size(),
						(wchar_t*)ret.data(), (int)ret.capacity());
				}
			}

			if(rval == 0)
			{
				int err = GetLastError();
				return L""; // TODO: check actual string size?
			}
			
			ret.resize(rval);
			return ret;
		}

		NETLIB_API std::string utf16_to_utf8(std::wstring const& _text)
		{
			std::string ret;
			ret.resize(_text.size()*2);

			int rval = WideCharToMultiByte(CP_UTF8, 0,
				_text.data(), (int)_text.size(), (char*)ret.data(), (int)ret.capacity(),
				nullptr, FALSE);

			if(rval == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				rval = WideCharToMultiByte(CP_UTF8, 0,
					_text.data(), (int)_text.size(), NULL, 0,
					nullptr, FALSE);
				if(rval)
				{
					ret.resize(rval+1);
					rval = WideCharToMultiByte(CP_UTF8, 0,
						_text.data(), (int)_text.size(),
						(char*)ret.data(), (int)ret.capacity(),
						nullptr, FALSE);
				}
			}

			if(rval == 0)
			{
				int err = GetLastError();
				return ""; // TODO: check actual string size?
			}
			
			ret.resize(rval);
			return ret;
		}
	}
}