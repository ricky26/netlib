#include "bitstream.h"
#include <string>

#pragma once

namespace netlib
{
	class socket_constructor_t;
	class pipe_constructor_t
	{
		friend class pipe;

		pipe_constructor_t(int _handle)
			: handle(_handle)
		{
		}

		int handle;
	};

	class socket;
	class NETLIB_API pipe: public bitstream
	{
	public:
		pipe();
		pipe(pipe_constructor_t const& _con);
		explicit pipe(int _handle);
		pipe(pipe &);
		virtual ~pipe();

		bool valid() const;
		int handle() const;
		int release();
		
		bool open(std::string const& _pipe);
		bool create(std::string const& _pipe);
		pipe_constructor_t accept();
		void close();

		virtual size_t read(void *_buffer, size_t _amt);
		virtual size_t write(const void *_buffer, size_t _amt);

		virtual socket_constructor_t read();
		virtual bool write(socket&);

		static bool init();
		static void think();
		static void shutdown();

	private:
		void *mInternal;
	};
}
