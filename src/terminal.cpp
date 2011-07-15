#include "netlib/terminal.h"
#include <vector>
#include <exception>
#include <string>
#include <cctype>

namespace netlib
{
	namespace terminal
	{
		static int get_colour_code(colour _c, bool &_bright)
		{
			switch(_c)
			{
			case black:
				_bright = false;
				return 0;

			case red:
				_bright = false;
				return 1;

			case green:
				_bright = false;
				return 2;

			case yellow:
				_bright = false;
				return 3;

			case blue:
				_bright = false;
				return 4;

			case cyan:
				_bright = false;
				return 5;

			case magenta:
				_bright = false;
				return 6;

			case grey:
				_bright = false;
				return 7;

			case dark_grey:
				_bright = true;
				return 0;

			case light_red:
				_bright = true;
				return 1;

			case light_green:
				_bright = true;
				return 2;

			case light_yellow:
				_bright = true;
				return 3;

			case light_blue:
				_bright = true;
				return 4;

			case light_cyan:
				_bright = true;
				return 5;

			case light_magenta:
				_bright = true;
				return 6;

			case white:
				_bright = true;
				return 7;
			}

			return 0;
		}

		NETLIB_API std::ostream &foreground(std::ostream &_strm, colour _c)
		{
			bool bright;
			int cc = 30 + get_colour_code(_c, bright);

			if(bright)
				return _strm << csi << "1;" << cc << 'm';
			else
				return _strm << csi << cc << 'm';
		}

		NETLIB_API std::ostream &background(std::ostream &_strm, colour _c)
		{
			bool bright;
			int cc = 40 + get_colour_code(_c, bright);

			return _strm << csi << cc << 'm';
		}

		NETLIB_API std::ostream &operator <<(std::ostream &_strm, colour _c)
		{
			return foreground(_strm, _c);
		}

		NETLIB_API std::ostream &esc(std::ostream &_strm)
		{
			_strm.put('\033');
			return _strm;
		}

		NETLIB_API std::ostream &csi(std::ostream &_strm)
		{
			esc(_strm);
			_strm.put('[');
			return _strm;
		}

		NETLIB_API std::ostream &bold(std::ostream &_strm)
		{
			_strm << csi << "1m";
			return _strm;
		}

		NETLIB_API std::ostream &italic(std::ostream &_strm)
		{
			_strm << csi << "3m";
			return _strm;
		}

		NETLIB_API std::ostream &underline(std::ostream &_strm)
		{
			_strm << csi << "4m";
			return _strm;
		}

		NETLIB_API std::ostream &regular(std::ostream &_strm)
		{
			_strm << csi << 'm';
			return _strm;
		}

		NETLIB_API std::ostream &font(std::ostream &_strm)
		{
			return font(_strm, 0);
		}

		NETLIB_API std::ostream &font(std::ostream &_strm, int _font)
		{
			if(_font >= 10)
				_font = 0;

			_strm << csi << (10+_font) << 'm';
			return _strm;
		}

		//
		// Cursor
		//

		NETLIB_API std::ostream &cr(std::ostream &_strm)
		{
			return _strm.put('\r');
		}

		NETLIB_API std::ostream &lf(std::ostream &_strm)
		{
			return _strm.put('\n');
		}

		NETLIB_API std::ostream &crlf(std::ostream &_strm)
		{
			_strm.put('\r');
			return _strm.put('\n');
		}
		
		NETLIB_API std::ostream &move(std::ostream &_strm, int _x, int _y)
		{
			if(_x < 0)
				_strm << csi << -_x << 'A';
			else
				_strm << csi << _x << 'B';

			if(_y < 0)
				_strm << csi << -_y << 'D';
			else
				_strm << csi << _y << 'C';

			return _strm;
		}

		NETLIB_API std::ostream &position(std::ostream &_strm, int _x, int _y)
		{
			return _strm << csi << _x << ';' << _y << 'H';
		}

		NETLIB_API std::ostream &up(std::ostream &_strm)
		{
			return _strm << csi << 'A';
		}

		NETLIB_API std::ostream &up(std::ostream &_strm, int _amt)
		{
			return _strm << csi << _amt << 'A';
		}

		NETLIB_API std::ostream &down(std::ostream &_strm)
		{
			return _strm << csi << 'B';
		}

		NETLIB_API std::ostream &down(std::ostream &_strm, int _amt)
		{
			return _strm << csi << _amt << 'B';
		}

		NETLIB_API std::ostream &left(std::ostream &_strm)
		{
			return _strm << csi << 'D';
		}

		NETLIB_API std::ostream &left(std::ostream &_strm, int _amt)
		{
			return _strm << csi << _amt << 'D';
		}

		NETLIB_API std::ostream &right(std::ostream &_strm)
		{
			return _strm << csi << 'C';
		}

		NETLIB_API std::ostream &right(std::ostream &_strm, int _amt)
		{
			return _strm << csi << _amt << 'C';
		}

		NETLIB_API std::ostream &show_cursor(std::ostream &_strm)
		{
			return _strm << csi << "?25l";
		}

		NETLIB_API std::ostream &hide_cursor(std::ostream &_strm)
		{
			return _strm << csi << "?25h";
		}
		
		NETLIB_API std::ostream &erase(std::ostream &_strm)
		{
			return erase(_strm, erase_all);
		}

		NETLIB_API std::ostream &erase(std::ostream &_strm, erase_mode _mode)
		{
			return _strm << csi << (int)_mode << 'J';
		}
		
		NETLIB_API std::ostream &erase_line(std::ostream &_strm)
		{
			return erase_line(_strm, erase_to_end);
		}

		NETLIB_API std::ostream &erase_line(std::ostream &_strm, erase_mode _mode)
		{
			return _strm << csi << (int)_mode << 'K';
		}

		//
		// Parse Input
		//
		
		void ansi_parser::accept(const char *_buffer, size_t _length)
		{
			mCache.insert(mCache.end(), _buffer, _buffer+_length);
			
			const char *start = &mCache.front();
			const char *end = start + mCache.size();
			const char *ptr = start;
			size_t data_len = 0;
			while(ptr < end)
			{
				if(ptr[0] == '\r')
				{
				}
				else if(ptr[0] == '\b' || ptr[0] == 0x7f)
				{
					if(data_len)
					{
						data(ptr - data_len, data_len);
						data_len = 0;
					}

					command(terminal::cmd_backspace, argument_list());
				}
				else if(ptr < end - 1
					&& ptr[0] == '\033' && ptr[1] == '[')
				{
					if(data_len)
					{
						data(ptr - data_len, data_len);
						data_len = 0;
					}
					
					const char *ptr2 = ptr + 2;
					argument_list args;
					if(ptr2 < end && std::isdigit(*ptr2))
					{
						while(ptr2 < end)
						{
							int val = 0;
							while(ptr2 < end && std::isdigit(ptr2[0]))
							{
								val *= 10;
								val += ptr2[0] - '0';
								ptr2++;
							}

							if(ptr2 >= end)
								break;

							args.push_back(val);

							if(ptr2[0] == ';')
								ptr2++;
						}
					}
					
					if(ptr2 >= end - 1)
					{
						mCache.erase(mCache.begin(),
							mCache.begin() - (ptr - start));
						return;
					}

					terminal::command cmd = (terminal::command)ptr2[1];
					ptr2++;

					command(cmd, args);

					ptr = ptr2;
				}
				else
					data_len++;

				ptr++;
			}

			if(data_len)
				data(ptr-data_len, data_len);
			mCache.clear();
		}
	}
}