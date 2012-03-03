#include "netlib/websocket.h"
#include "netlib/socket.h"
#include <sstream>
#include <memory>
#include <cstring>

namespace netlib
{
	namespace websocket
	{
		//
		// frame
		//

		frame::frame(netlib::websocket::opcode _type, int _flags,
			std::string const& _data, uint32_t _mask)
			: mOpCode(_type), mData(_data), mMask(_mask), mFlags(_flags)
		{
		}

		bool frame::valid() const
		{
			return mOpCode != 0;
		}

		opcode frame::opcode() const
		{
			return mOpCode;
		}

		int frame::flags() const
		{
			return mFlags;
		}

		std::string frame::data() const
		{
			return mData;
		}

		uint32_t frame::mask() const
		{
			return mMask;
		}

		//
		// websocket
		//

		websocket::websocket(netlib::socket *_sock)
			: mSocket(_sock)
		{
		}
		
		websocket::websocket(websocket &_b)
		{
			mSocket = _b.mSocket;
			_b.mSocket = NULL;
		}
		
		websocket::~websocket()
		{
			close();
		}

		bool websocket::valid() const
		{
			return mSocket && mSocket->valid();
		}

		netlib::socket *websocket::socket() const
		{
			return mSocket;
		}
		
		netlib::socket *websocket::release()
		{
			netlib::socket *ret = mSocket;
			mSocket = NULL;
			return ret;
		}

		frame websocket::read()
		{
			if(!valid())
				return frame();

			uint8_t control[2];
			
			opcode op=op_cont;
			int flags;
			std::stringstream strstr;
			uint32_t mask = 0;

			while(true)
			{
				if(!mSocket->read_block(control))
					return frame();

				uint32_t len = control[1] & 0x7f;
				if(len == 126)
				{
					uint16_t sz16;
					if(!mSocket->read_block(sz16))
						return frame();

					len = sz16;
				}
				else if(len == 127)
				{
					// 64-bit length, we don't like these!
					return frame();
				}

				bool hasmask = (control[1] & 0x80) != 0;
				bool fin = (control[0] & netlib::websocket::fin) != 0;

				if(!op)
				{
					op = (opcode)(control[0] & 0xf);
					flags = control[0] & 0x70; // Don't pass fin down the line.
				}

				if(hasmask)
				{
					if(!mSocket->read_block(mask))
						return frame();
				}

				size_t done = 0;
				char buff[1024];

				uint8_t *maskp = (uint8_t*)&mask;

				while(done < len)
				{
					size_t sz = mSocket->read(buff);
					if(sz == 0)
						return frame();

					for(size_t i = 0; i < sz; i++)
						buff[i] ^= maskp[i%4];

					done += sz;

					strstr.write(buff, sz);
				}

				if(fin)
					break;
			}

			return frame(op, flags, strstr.str(), mask);
		}

		bool websocket::write(frame const& _frm)
		{
			if(!valid())
				return false;

			std::string data = _frm.data();

			uint8_t control[2];

			control[0] = (_frm.flags() & 0xF0) | (_frm.opcode() & 0xf) | fin;
			control[1] = (_frm.mask() ? 0x80 : 0);
			
			size_t sz = data.length();
			if(sz > 0xffff)
				return false; // TODO: 64-bit shizz.
			else if(sz > 126)
			{
				uint16_t len = (uint16_t)data.size();
				control[1] |= 126;

				if(!mSocket->write_block(control))
					return false;

				if(!mSocket->write_block(len))
					return false;
			}
			else
			{
				control[1] |= data.size();

				if(!mSocket->write_block(control))
					return false;
			}

			if(_frm.mask())
			{
				uint32_t mask = _frm.mask();
				if(!mSocket->write_block(mask))
					return false;

				size_t amt = (data.size()
					+sizeof(uint32_t)-1)/sizeof(uint32_t);
				uint32_t *buf = new uint32_t[amt];
				std::auto_ptr<uint32_t> bufptr(buf); // Make sure buf is deleted

				std::memcpy(buf, data.data(), data.size());

				for(size_t i = 0; i < amt; i++)
					buf[i] ^= mask;

				if(!mSocket->write_block(buf, data.size()))
					return false;
			}
			else
			{
				if(!mSocket->write_block(data.data(), data.size()))
					return false;
			}

			return true;
		}

		bool websocket::write(std::string const& _data, opcode _code, int _flags)
		{
			return write(frame(_code, _flags, _data));
		}

		void websocket::close(std::string const& _msg)
		{
			if(!valid())
				return;

			write(_msg, op_close);
			mSocket->close();
		}

		void websocket::ping(std::string const& _text)
		{
		}
	}
}
