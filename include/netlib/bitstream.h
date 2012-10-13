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

		template<size_t BufferSize>
		std::string read_all()
		{
			std::string ret;
			ret.resize(BufferSize);

			size_t amt;
			while((amt = read(const_cast<char*>(ret.data()+ret.size()-BufferSize),
				BufferSize)) > 0)
				ret.resize(ret.size() + amt);

			ret.resize(ret.size() - BufferSize);
			return ret;
		}

		inline std::string read_all()
		{
			return read_all<16*1024>();
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
		inline bool copy(bitstream &_to)
		{
			char buf[_bufferSize];
			size_t amt;

			while((amt = read(buf, sizeof(buf))), amt > 0)
				if(!_to.write_block(buf, amt))
					return false;
			return true;
		}

		inline bool copy(bitstream &_to)
		{
			return copy<4096>(_to);
		}

	protected:
		bitstream();
	};
}
