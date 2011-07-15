#include "netlib/md5.h"
#include <Windows.h>
#include <WinCrypt.h>

namespace netlib
{
	//
	// md5_internal
	//

	struct md5_internal
	{
		HCRYPTPROV provider;
		HCRYPTHASH hash;

		md5_internal()
		{
			provider = NULL;
			hash = NULL;
		}

		static inline md5_internal *get(void *_p)
		{
			return static_cast<md5_internal*>(_p);
		}
	};

	//
	// md5
	//

	md5::md5()
	{
		mInternal = new md5_internal();
		begin();
	}

	md5::~md5()
	{
		if(mInternal)
		{
			md5_internal *mi = md5_internal::get(mInternal);
			mInternal = NULL;

			if(mi->hash)
				CryptDestroyHash(mi->hash);

			if(mi->provider)
				CryptReleaseContext(mi->provider, 0);

			delete mi;
		}
	}

	bool md5::begin()
	{
		md5_internal *mi = md5_internal::get(mInternal);
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

		return CryptCreateHash(mi->provider, CALG_MD5, NULL, 0, &mi->hash) == TRUE;
	}

	bool md5::update(void *_ptr, size_t _sz)
	{
		md5_internal *mi = md5_internal::get(mInternal);
		if(!mi || !mi->hash)
			return false;

		return CryptHashData(mi->hash, (const BYTE*)_ptr, _sz, 0) == TRUE;
	}

	std::string md5::compute()
	{
		md5_internal *mi = md5_internal::get(mInternal);
		if(!mi || !mi->hash)
			return std::string();

		std::string ret;
		ret.resize(16, 0);

		DWORD len = ret.size();
		if(!CryptGetHashParam(mi->hash, HP_HASHVAL, (BYTE*)ret.data(), &len, 0))
			return std::string();

		if(len != ret.size())
			ret.resize(len);

		return ret;
	}
}