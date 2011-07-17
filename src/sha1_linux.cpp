#include "netlib/sha1.h"
#include <openssl/sha.h>

namespace netlib
{
	//
	// sha1
	//

	sha1::sha1()
	{
		mInternal = new SHA_CTX;
		begin();
	}

	sha1::~sha1()
	{
		if(mInternal)
			delete (SHA_CTX*)mInternal;
	}

	bool sha1::begin()
	{
		return SHA1_Init((SHA_CTX*)mInternal) >= 0;
	}

	bool sha1::update(void *_ptr, size_t _sz)
	{
		return SHA1_Update((SHA_CTX*)mInternal, _ptr, _sz) >= 0;
	}

	std::string sha1::compute()
	{
		std::string ret;
		ret.resize(20, 0);

		if(SHA1_Final((unsigned char*)ret.data(), (SHA_CTX*)mInternal) < 0)
			ret.resize(0);

		return ret;
	}
}
