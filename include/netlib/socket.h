#include "bitstream.h"
#include <string>

#pragma once

namespace netlib
{
	enum address_family
	{
		af_inet,
		af_unix,
	};

	enum socket_type
	{
		sock_stream,
		sock_dgram,
	};

	enum socket_protocol
	{
		prot_any,
	};

	class NETLIB_API socket: public bitstream
	{
	public:
		using bitstream::read;
		using bitstream::read_block;
		using bitstream::write;
		using bitstream::write_block;

		socket();
		socket(address_family _af, socket_type _sock=sock_stream,
			socket_protocol _prot=prot_any);
		socket(socket const&);
		socket(socket&&);
		explicit socket(int _sock);
		virtual ~socket();

		bool create(address_family _af=af_inet, socket_type _sock=sock_stream,
			socket_protocol _prot=prot_any);

		bool valid() const;
		int handle() const;
		int release();
		
		bool connect(std::string const& _host, int _port);
		bool listen(int _port, int _amt=15);
		socket accept();
		void close();

		virtual size_t read(void *_buffer, size_t _amt) override;		
		virtual size_t write(const void *_buffer, size_t _amt) override;

		static bool init();
		static void think();
		static void shutdown();

	private:
		void *mInternal;
	};
}
