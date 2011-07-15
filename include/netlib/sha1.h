#include "crypto.h"

#pragma once

namespace netlib
{
	class NETLIB_API sha1: public crypto::hash
	{
	public:
		using crypto::hash::update;

		sha1();
		~sha1();

		bool begin();
		bool update(void *_data, size_t _sz);
		std::string compute();

	private:
		void *mInternal;
	};
}