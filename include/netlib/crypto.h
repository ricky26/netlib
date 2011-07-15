#include "netlib.h"
#include <string>

#pragma once

namespace netlib
{
	namespace crypto
	{
		class NETLIB_API hash
		{
		public:
			virtual ~hash();

			virtual bool begin() = 0;
			virtual bool update(void *_data, size_t _sz) = 0;
			virtual bool update(std::string const& _str);
			virtual std::string compute() = 0;

		protected:
			hash();
		};

		class NETLIB_API encoding
		{
		public:
			virtual ~encoding();
			
			virtual std::string encode(std::string const& _str) = 0;
			virtual std::string decode(std::string const& _str) = 0;

		protected:
			encoding();
		};
	}
}
