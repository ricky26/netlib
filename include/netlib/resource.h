#include "netlib.h"
#include "bitstream.h"
#include "module.h"
#include <string>

#pragma once

namespace netlib
{
	class NETLIB_API resource: public bitstream
	{
	public:
		explicit resource(module const& _m, std::string const& _name);
		resource();
		resource(resource const& _res);
		~resource();

		using bitstream::read;
		using bitstream::read_block;
		using bitstream::write;
		using bitstream::write_block;

		bool valid() const;
		bool open(module const& _m, std::string const& _nm);
		void close();

		virtual size_t read(void *_buffer, size_t _amt) override;		
		virtual size_t write(const void *_buffer, size_t _amt) override;

	private:
		void *mInternal;
	};
}
