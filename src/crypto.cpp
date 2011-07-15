#include "netlib/crypto.h"

namespace netlib
{
	namespace crypto
	{
		//
		// hash
		//

		hash::hash()
		{
		}

		hash::~hash()
		{
		}

		bool hash::update(std::string const& _str)
		{
			return update((void*)_str.data(), _str.length());
		}

		//
		// encoding
		//
		
		encoding::encoding()
		{
		}

		encoding::~encoding()
		{
		}
	}
}