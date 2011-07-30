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

	class socket_constructor_t
	{
		friend class pipe;
		friend class socket;

		inline socket_constructor_t(void *_val)
			: value(_val) {}

		void *value;
	};

	class NETLIB_API socket: public bitstream
	{
	public:
		using bitstream::read;
		using bitstream::read_block;
		using bitstream::write;
		using bitstream::write_block;

		socket(address_family _af=af_inet,
			socket_type _sock=sock_stream, socket_protocol _prot=prot_any);
		socket(socket_constructor_t const& _con);
		explicit socket(int _sock);
		socket(socket &);
		virtual ~socket();

		bool valid() const;
		int handle() const;
		int release();

		socket_constructor_t returnable_value();
		
		bool connect(std::string const& _host, int _port);
		bool listen(int _port, int _amt=15);
		socket_constructor_t accept();
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
