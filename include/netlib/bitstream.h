#include "netlib.h"
#include <string>

#pragma once

namespace netlib
{
	class NETLIB_API bitstream
	{
	public:
		virtual ~bitstream();

		// read

		virtual size_t read(void *_buffer, size_t _amt) = 0;

		template<typename T>
		size_t read(T &_out)
		{
			return read(&_out, sizeof(_out));
		}

		// read_block

		virtual bool read_block(void *_buffer, size_t _amt);

		template<typename T>
		bool read_block(T &_out)
		{
			return read_block(&_out, sizeof(_out));
		}

		// write
		
		virtual size_t write(const void *_buffer, size_t _amt) = 0;
		virtual size_t write(std::string const& _str);

		template<typename T>
		size_t write(T const& _out)
		{
			return write(&_out, sizeof(_out));
		}

		// write_block

		virtual bool write_block(const void *_buffer, size_t _amt);
		virtual bool write_block(std::string const& _str);

		template<typename T>
		bool write_block(T const& _out)
		{
			return write_block(&_out, sizeof(_out));
		}

		virtual void flush();

		//
		// copy
		//
		
		template<size_t _bufferSize>
		inline void copy(bitstream &_to)
		{
			char buf[_bufferSize];
			size_t amt;

			while((amt = read(buf, sizeof(buf))), amt > 0)
				_to.write(buf, amt);
		}

		inline void copy(bitstream &_to)
		{
			return copy<4096>(_to);
		}

	protected:
		bitstream();
	};
}