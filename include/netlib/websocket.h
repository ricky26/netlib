#include "bitstream.h"
#include <string>
#include <stdint.h>

#pragma once

namespace netlib
{
	class socket;

	class NETLIB_API websocket
	{
	public:
		enum opcode
		{
			// Message OpCodes
			op_cont		= 0,
			op_text		= 1,
			op_binary	= 2,
			op_r1		= 3,
			op_r2		= 4,
			op_r3		= 5,
			op_r4		= 6,
			op_r5		= 7,

			// Control OpCodes
			op_close	= 8,
			op_ping		= 9,
			op_pong		= 10,
			op_rc1		= 11,
			op_rc2		= 12,
			op_rc3		= 13,
			op_rc4		= 14,
			op_rc5		= 15,
		};

		enum flag
		{
			fin		= 1 << 7,
			rsv1	= 1 << 6,
			rsv2	= 1 << 5,
			rsv3	= 1 << 4,
			mask	= 1 << 8,
		};

		class NETLIB_API frame
		{
		public:
			frame(netlib::websocket::opcode _type=op_cont, int flags=0, std::string const& _data="", uint32_t _mask=0);

			bool valid() const;
			netlib::websocket::opcode opcode() const;
			int flags() const;
			std::string data() const;
			uint32_t mask() const;

			static inline frame text(std::string const& _text)
			{
				return frame(op_text, 0, _text);
			}

		private:
			netlib::websocket::opcode mOpCode;
			int mFlags;
			uint32_t mMask;
			std::string mData;
		};

		websocket();
		explicit websocket(netlib::socket &_sock);
		websocket(websocket&);
		websocket(websocket&&);
		~websocket();

		bool valid() const;
		netlib::socket *socket() const;
		netlib::socket *release();
		
		frame read();
		bool write(frame const& _frm);
		bool write(std::string const& _data,
				netlib::websocket::opcode _code=op_text,
				int _flags=0);
		void close(std::string const& _msg="");

		void ping(std::string const& _text="");

	private:
		netlib::socket *mSocket;
	};
}
