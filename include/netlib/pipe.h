#include "bitstream.h"
#include <string>

#pragma once

namespace netlib
{
	class socket;
	class NETLIB_API pipe: public bitstream
	{
	public:
		pipe();
		pipe(pipe const&);
		pipe(pipe &&);
		explicit pipe(int _handle);
		virtual ~pipe();

		bool valid() const;
		int handle() const;
		int release();
		
		bool open(std::string const& _pipe);
		bool create(std::string const& _pipe);
		pipe accept();
		void close();

		virtual size_t read(void *_buffer, size_t _amt) override;
		virtual size_t write(const void *_buffer, size_t _amt) override;

		virtual socket read();
		virtual bool write(socket&);

		static bool init();
		static void think();
		static void shutdown();

	private:
		void *mInternal;
	};
}
