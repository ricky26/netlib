#include "netlib/sha1.h"
#include <Windows.h>
#include <WinCrypt.h>

namespace netlib
{
	//
	// sha1_internal
	//

	struct sha1_internal
	{
		HCRYPTPROV provider;
		HCRYPTHASH hash;

		sha1_internal()
		{
			provider = NULL;
			hash = NULL;
		}

		static inline sha1_internal *get(void *_p)
		{
			return static_cast<sha1_internal*>(_p);
		}
	};

	//
	// sha1
	//

	sha1::sha1()
	{
		mInternal = new sha1_internal();
		begin();
	}

	sha1::~sha1()
	{
		if(mInternal)
		{
			sha1_internal *mi = sha1_internal::get(mInternal);
			mInternal = NULL;

			if(mi->hash)
				CryptDestroyHash(mi->hash);

			if(mi->provider)
				CryptReleaseContext(mi->provider, 0);

			delete mi;
		}
	}

	bool sha1::begin()
	{
		sha1_internal *mi = sha1_internal::get(mInternal);
		if(!mi)
			return false;
		
		if(!mi->provider)
		{
			if(!CryptAcquireContextA(&mi->provider, NULL, NULL,
				PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
				return false; // TODO: sort out exceptions!
		}

		if(mi->hash)
		{
			CryptDestroyHash(mi->hash);
			mi->hash = NULL;
		}

		return CryptCreateHash(mi->provider, CALG_SHA1, NULL, 0, &mi->hash) == TRUE;
	}

	bool sha1::update(void *_ptr, size_t _sz)
	{
		sha1_internal *mi = sha1_internal::get(mInternal);
		if(!mi || !mi->hash)
			return false;

		return CryptHashData(mi->hash, (const BYTE*)_ptr, _sz, 0) == TRUE;
	}

	std::string sha1::compute()
	{
		sha1_internal *mi = sha1_internal::get(mInternal);
		if(!mi || !mi->hash)
			return std::string();

		std::string ret;
		ret.resize(20, 0);

		DWORD len = ret.size();
		if(!CryptGetHashParam(mi->hash, HP_HASHVAL, (BYTE*)ret.data(), &len, 0))
			return std::string();

		if(len != ret.size())
			ret.resize(len);

		return ret;
	}
}