#include "netlib.h"
#include <string>

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

		struct utf8
		{
			typedef std::string storage_t;
		};

		struct utf16
		{
			typedef std::wstring storage_t;
		};

		template<typename d_t, typename s_t>
		struct convert_encoding
		{
			static inline void convert()
			{
				static_assert(false, "convert_encoding not implemented for s_t/d_t.");
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

		template<typename dest_t, typename src_t>
		static inline typename dest_t::storage_t convert(typename src_t::storage_t const& _str)
		{
			return convert_encoding<dest_t, src_t>::convert(_str);
		}
	}
}
