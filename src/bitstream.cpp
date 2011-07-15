#include "netlib/bitstream.h"

namespace netlib
{
	bitstream::bitstream()
	{
	}

	bitstream::~bitstream()
	{
	}

	bool bitstream::read_block(void *_buffer, size_t _amt)
	{
		size_t amtDone = 0;
		while(amtDone < _amt)
		{
			size_t amt = read(((char*)_buffer)+amtDone, _amt-amtDone);
			if(amt == 0)
				return false;

			amtDone += amt;
		}

		return true;
	}

	size_t bitstream::write(std::string const& _str)
	{
		return write(_str.data(), _str.length());
	}
		
	bool bitstream::write_block(const void *_buffer, size_t _amt)
	{
		size_t amtDone = 0;
		while(amtDone < _amt)
		{
			size_t amt = write(((char*)_buffer)+amtDone, _amt-amtDone);
			if(amt == 0)
				return false;

			amtDone += amt;
		}

		return true;
	}

	bool bitstream::write_block(std::string const& _str)
	{
		return write_block(_str.data(), _str.length());
	}

	void bitstream::flush()
	{
	}
}
