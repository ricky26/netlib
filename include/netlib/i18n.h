#include "netlib.h"
#include <string>
#include <cassert>

#pragma once

namespace netlib
{
	namespace i18n
	{
		/** Convert a UTF-8 string into UTF-16. */
		NETLIB_API std::wstring utf8_to_utf16(std::string const& _str);

		/** Convert a UTF-16 string into UTF-16. */
		NETLIB_API std::string utf16_to_utf8(std::wstring const& _wstr);

		//
		// Template magic
		//

		template<typename U, typename S>
		struct encoding_base
		{
			typedef S storage_t;
		};

		struct utf8: public encoding_base<utf8, std::string> {};
		struct utf16: public encoding_base<utf16, std::wstring> {};

		template<typename d_t, typename s_t>
		struct convert_encoding
		{
			static inline void convert()
			{
				assert(false && "convert_encoding not implemented for s_t/d_t.");
			}
		};

		template<>
		struct convert_encoding<utf16, utf8>
		{
			static inline std::wstring convert(std::string const& _str)
			{
				return utf8_to_utf16(_str);
			}
		};

		template<>
		struct convert_encoding<utf8, utf16>
		{
			static inline std::string convert(std::wstring const& _str)
			{
				return utf16_to_utf8(_str);
			}
		};

		template<typename T>
		struct convert_encoding<T, T>
		{
			typedef typename T::storage_t str_t;

			static inline str_t convert(str_t const& _str)
			{
				return _str;
			}
		};

		template<typename D, typename S>
		typename D::storage_t convert(typename S::storage_t const& _value)
		{
			return convert_encoding<D, S>::convert(_value);
		}
	}
}
