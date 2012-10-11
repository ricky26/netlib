#include "netlib.h"
#include "file.h"
#include <string>

#pragma once

namespace netlib
{
	class NETLIB_API module
	{
	public:
		module();
		explicit module(void *_h); // From handle
		module(module const& _mod);
		module(module &&_mod);
		module(std::string const& _name);
		virtual ~module();

		void *handle() const;

		bool valid() const;
		bool open(std::string const& _name);
		void close();

		void *symbol(std::string const& _nm) const;

		template<typename T>
		T symbol(std::string const& _nm) const
		{
			return static_cast<T>(symbol(_nm));
		}

		netlib::file file() const;
		
		module &operator =(module const& _b);
		inline bool operator ==(module const& _b) const { return mInternal == _b.mInternal; }
		inline bool operator !=(module const& _b) const { return mInternal != _b.mInternal; }

	private:
		void *mInternal;
	};

	NETLIB_API module netlib_module();
	NETLIB_API module main_module();
}
