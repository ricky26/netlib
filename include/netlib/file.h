#include "bitstream.h"
#include <string>
#include <stdint.h>

#pragma once

namespace netlib
{
	class NETLIB_API file: public bitstream
	{
	public:
		typedef int64_t seek_pos_t;

		enum mode_t
		{
			mode_read = 1,		// 001
			mode_write = 2,		// 010
			mode_append = 4,	// 100

			mode_readwrite = mode_read | mode_write,
			mode_rwa = mode_readwrite | mode_append,

			rwa_mask = 7,

			mode_open = 8,		// 01x
			mode_create = 16,	// 10x
			mode_open_or_create = mode_open | mode_create,

			open_mask = mode_open_or_create,
		};

		enum seek_t
		{
			from_start,
			from_end,
			relative,
		};

		file();
		file(file const&);
		file(std::string const& _path, int _mode=mode_open|mode_read);
		virtual ~file();

		bool valid() const;
		bool open(std::string const& _path, int _mode);
		void close();
		
		virtual size_t read(void *_buffer, size_t _amt) override;
		virtual size_t write(const void *_buffer, size_t _amt) override;
		bool seek(seek_pos_t _pos, seek_t _mode);

		virtual void flush();

		static bool init();
		static void think();
		static void shutdown();

	private:
		void *mInternal;
	};
}
