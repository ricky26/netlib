#include "netlib/md5.h"
#include <openssl/md5.h>

namespace netlib
{
	//
	// md5
	//

	md5::md5()
	{
		mInternal = new MD5_CTX;
		begin();
	}

	md5::~md5()
	{
		if(mInternal)
			delete (MD5_CTX*)mInternal;
	}

	bool md5::begin()
	{
		return MD5_Init((MD5_CTX*)mInternal) >= 0;
	}

	bool md5::update(void *_ptr, size_t _sz)
	{
		return MD5_Update((MD5_CTX*)mInternal, _ptr, _sz) >= 0;
	}

	std::string md5::compute()
	{
		std::string ret;
		ret.resize(20, 0);

		if(MD5_Final((unsigned char*)ret.data(), (MD5_CTX*)mInternal) < 0)
			ret.resize(0);

		return ret;
	}
}
