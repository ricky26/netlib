#include "netlib.h"
#include <string>

#pragma once

namespace netlib
{
	class NETLIB_API module
	{
	public:
		module();
		module(module const& _mod);
		module(std::string const& _name);
		virtual ~module();

		bool valid() const;
		bool open(std::string const& _name);
		void close();

		void *symbol(std::string const& _nm) const;

		template<typename T>
		T symbol(std::string const& _nm) const
		{
			return static_cast<T>(symbol(_nm));
		}

	private:
		void *mInternal;
	};
}
