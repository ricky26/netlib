#include "netlib/base64.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

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
		BIO *b64 = BIO_new(BIO_f_base64());
		BIO *bmem = BIO_new(BIO_s_mem());
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

		b64 = BIO_push(b64, bmem);
		BIO_write(b64, _str.data(), _str.length());
		BIO_flush(b64);

		BUF_MEM *buf;
		BIO_get_mem_ptr(b64, &buf);

		std::string ret(buf->data, buf->data+buf->length);

		BIO_free_all(b64);
		return ret;
	}

	std::string base64::decode(std::string const& _str)
	{
		std::string ret;
		ret.resize(_str.length());

		BIO *b64 = BIO_new(BIO_f_base64());
		BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

		BIO *bmem = BIO_new_mem_buf((void*)_str.data(), _str.length());
		b64 = BIO_push(b64, bmem);

		int len = BIO_read(b64, (void*)ret.data(), ret.length());
		if(len <= 0)
			ret.resize(0);
		else
			ret.resize(len);

		return ret;
	}
}
