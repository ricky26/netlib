#include "bitstream.h"
#include <string>
#include <functional>
#include <stdint.h>

#pragma once

namespace netlib
{
	class NETLIB_API file: public bitstream
	{
	public:
		typedef int64_t seek_pos_t;
		
		using bitstream::read;
		using bitstream::read_block;
		using bitstream::write;
		using bitstream::write_block;

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
		file(file &&);
		file(std::string const& _path, int _mode=mode_open|mode_read);
		explicit file(void *_handle);
		virtual ~file();

		bool valid() const;
		bool open(std::string const& _path, int _mode=mode_open|mode_read);
		void close();

		uint64_t size();
		
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

	class NETLIB_API directory
	{
	public:
		typedef std::function<void(std::string const& _pth)> list_t;

		directory();
		directory(std::string const& _dir);

		inline std::string path() const { return mPath; }

		bool valid() const;
		bool create();

		bool list(list_t _cb);

		std::string child(std::string const& _nm) const;

		static bool exists(std::string const& _path);
		static void create(std::string const& _path);
		static void create_parents(std::string const& _path);
		static bool list(std::string const& _path, list_t _cb);

		static std::pair<std::string, std::string> split(std::string const& _p);
		static std::string combine(std::string const& _a, std::string const& _b);
		
		static directory documents();
		static directory home();

	private:
		std::string mPath;
	};

	class NETLIB_API directory_watcher
	{
	public:
		struct event
		{
			enum type
			{
				invalid = 0,
				added,
				removed,
				modified,
				renamed
			} type;

			std::string name;
			std::string old_name;

			event();
		};

		directory_watcher();
		directory_watcher(directory_watcher const&);
		directory_watcher(directory_watcher &&);
		directory_watcher(directory const& _dir);
		~directory_watcher();

		bool valid() const;
		bool create(directory const& _dir);

		event wait();

	private:
		void *mInternal;
	};
}
