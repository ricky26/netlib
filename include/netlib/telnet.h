#include "netlib.h"
#include "socket.h"
#include "terminal.h"
#include <iostream>
#include <vector>

namespace netlib
{
	namespace telnet
	{
		enum option: unsigned char
		{
			echo = 1,
			supress_go_ahead = 3,
			status = 5,
			carriage_return = 10,
			window_size = 31,
			line_feed = 16,
			line_mode = 34,
		};

		NETLIB_API std::ostream &iac(std::ostream&);
		NETLIB_API std::ostream &ds(std::ostream&);
		NETLIB_API std::ostream &dr(std::ostream&);
		
		NETLIB_API std::ostream &sub_begin(std::ostream&);
		NETLIB_API std::ostream &sub_begin(std::ostream&, option, bool);

		NETLIB_API std::ostream &sub_end(std::ostream&);
		NETLIB_API std::ostream &sub(std::ostream&, option, bool, void*, size_t);
		
		template <option opt, bool x>
		std::ostream &sub_begin(std::ostream &_strm)
		{
			return sub_begin(_strm, opt, x);
		}

		template <option opt, bool x, char y>
		std::ostream &sub(std::ostream &_strm)
		{
			char ch = y;
			return sub(_strm, opt, x, &ch, sizeof(ch));
		}
		
		NETLIB_API std::ostream &will(std::ostream&);
		NETLIB_API std::ostream &will(std::ostream&, option);

		template <option _o>
		inline std::ostream &will(std::ostream &_strm)
		{
			return will(_strm, _o);
		}
		
		NETLIB_API std::ostream &wont(std::ostream&);
		NETLIB_API std::ostream &wont(std::ostream&, option);

		template <option _o>
		inline std::ostream &wont(std::ostream &_strm)
		{
			return wont(_strm, _o);
		}
		
		NETLIB_API std::ostream &enable(std::ostream&);
		NETLIB_API std::ostream &enable(std::ostream&, option);

		template <option _o>
		inline std::ostream &enable(std::ostream &_strm)
		{
			return enable(_strm, _o);
		}
		
		NETLIB_API std::ostream &disable(std::ostream&);
		NETLIB_API std::ostream &disable(std::ostream&, option);

		template <option _o>
		inline std::ostream &disable(std::ostream &_strm)
		{
			return disable(_strm, _o);
		}

		//
		// Input
		//

		enum command: unsigned char
		{
			cmd_se = 240,
			cmd_sb = 250,

			cmd_do = 251,
			cmd_dont = 252,
			cmd_will = 253,
			cmd_wont = 254,

			cmd_iac = 0xff,
		};

		class NETLIB_API telnet_parser
		{
		public:
			void accept(const char *_buffer, size_t _length);

			virtual void data(const char *_data, size_t _length) = 0;
			virtual void command(command _cmd, option _option) = 0;
			virtual void configure(option _option, bool _val_req, const char *_buffer, size_t _length) = 0;

		private:
			std::vector<char> mCache;
		};
	}
}