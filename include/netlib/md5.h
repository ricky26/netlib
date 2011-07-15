#include "crypto.h"

#pragma once

namespace netlib
{
	class NETLIB_API md5: public crypto::hash
	{
	public:
		using crypto::hash::update;

		md5();
		~md5();

		bool begin();
		bool update(void *_data, size_t _sz);
		std::string compute();

	private:
		void *mInternal;
	};
}
