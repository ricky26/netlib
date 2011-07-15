#include "netlib.h"
#include <iostream>
#include <vector>

#pragma once

namespace netlib
{
	namespace terminal
	{
		//
		// Formatting
		//

		enum colour
		{
			black,
			red,
			green,
			yellow,
			blue,
			cyan,
			magenta,
			grey,
			dark_grey,
			light_red,
			light_green,
			light_yellow,
			light_blue,
			light_cyan,
			light_magenta,
			white
		};
		
		NETLIB_API std::ostream &foreground(std::ostream &, colour);

		template <colour _c>
		inline std::ostream &foreground(std::ostream &_strm)
		{
			return foreground(_strm, _c);
		}

		NETLIB_API std::ostream &background(std::ostream &, colour);

		template <colour _c>
		inline std::ostream &background(std::ostream &_strm)
		{
			return background(_strm, _c);
		}

		NETLIB_API std::ostream &operator <<(std::ostream &, colour);

		NETLIB_API std::ostream &esc(std::ostream &);
		NETLIB_API std::ostream &csi(std::ostream &);
		NETLIB_API std::ostream &bold(std::ostream &);
		NETLIB_API std::ostream &italic(std::ostream &);
		NETLIB_API std::ostream &underline(std::ostream &);
		NETLIB_API std::ostream &regular(std::ostream &);
		
		NETLIB_API std::ostream &font(std::ostream &);
		NETLIB_API std::ostream &font(std::ostream &, int _font);

		template <int t>
		inline std::ostream &font(std::ostream &_strm)
		{
			return font(_strm, t);
		}

		//
		// Cursor
		//
		
		NETLIB_API std::ostream &cr(std::ostream &);
		NETLIB_API std::ostream &lf(std::ostream &);
		NETLIB_API std::ostream &crlf(std::ostream &);

		NETLIB_API std::ostream &move(std::ostream &, int _x, int _y);
		
		template <int x, int y>
		inline std::ostream &move(std::ostream &_strm)
		{
			return move(_strm, x, y);
		}

		NETLIB_API std::ostream &position(std::ostream &, int _x, int _y);
		
		template <int x, int y>
		inline std::ostream &position(std::ostream &_strm)
		{
			return position(_strm, x, y);
		}
		
		NETLIB_API std::ostream &up(std::ostream &);
		NETLIB_API std::ostream &up(std::ostream &, int _amt);

		template <int x>
		inline std::ostream &up(std::ostream &_strm)
		{
			return up(_strm, x);
		}

		NETLIB_API std::ostream &down(std::ostream &);
		NETLIB_API std::ostream &down(std::ostream &, int _amt);

		template <int x>
		inline std::ostream &down(std::ostream &_strm)
		{
			return down(_strm, x);
		}

		NETLIB_API std::ostream &left(std::ostream &);
		NETLIB_API std::ostream &left(std::ostream &, int _amt);

		template <int x>
		inline std::ostream &left(std::ostream &_strm)
		{
			return left(_strm, x);
		}

		NETLIB_API std::ostream &right(std::ostream &);
		NETLIB_API std::ostream &right(std::ostream &, int _amt);

		template <int x>
		inline std::ostream &right(std::ostream &_strm)
		{
			return right(_strm, x);
		}

		NETLIB_API std::ostream &show_cursor(std::ostream &);
		NETLIB_API std::ostream &hide_cursor(std::ostream &);
		
		enum erase_mode
		{
			erase_to_end = 0,
			erase_from_start = 1,
			erase_all = 2,
		};

		NETLIB_API std::ostream &erase(std::ostream &);
		NETLIB_API std::ostream &erase(std::ostream &, erase_mode _mode=erase_all);

		template <erase_mode _mode>
		inline std::ostream &erase(std::ostream &_strm)
		{
			return erase(_strm, _mode);
		}

		NETLIB_API std::ostream &erase_line(std::ostream &);
		NETLIB_API std::ostream &erase_line(std::ostream &, erase_mode _mode=erase_all);

		template <erase_mode _mode>
		inline std::ostream &erase_line(std::ostream &_strm)
		{
			return erase_line(_strm, _mode);
		}

		//
		// Parsing input
		//

		enum command: char
		{
			cmd_up = 'A',
			cmd_down = 'B',
			cmd_left = 'D',
			cmd_right = 'C',

			cmd_backspace = '\b',
		};

		class NETLIB_API ansi_parser
		{
		public:
			typedef std::vector<int> argument_list;

			void accept(const char *_buffer, size_t _length);

			virtual void data(const char *_buffer, size_t _length) = 0;
			virtual void command(command _cmd, argument_list _option) = 0;

		private:
			std::vector<char> mCache;
		};
	}
}