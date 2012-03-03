#include "netlib/irc.h"
#include <sstream>
#include <cstring>
#include <cstdarg>

namespace netlib
{
	irc_message::irc_message()
	{
	}

	irc_message::irc_message(std::string const& _command,
		parameters_t const& _params,
		std::string const &_src):
			mSource(_src),
			mCommand(_command),
			mParameters(_params)
	{
	}

	bool irc_message::valid() const
	{
		return !mCommand.empty();
	}

	std::string irc_message::source() const
	{
		return mSource;
	}

	std::string irc_message::command() const
	{
		return mCommand;
	}

	const irc_message::parameters_t &irc_message::parameters() const
	{
		return mParameters;
	}
		
	size_t irc_message::parse(const char *_data, size_t _len,
		irc_message *_msg)
	{
		// TODO: deal with 'pre-parsed' data moar efficiently.

		const char *end = std::strchr(_data, '\n');
		if(!end)
			return 0;
		
		const char *ptr = _data;
		size_t done = 1+end-ptr;
		if(end > ptr && *(end-1) == '\r')
			end--;

		if(*ptr == ':')
		{
			// We have a source name.

			std::stringstream ss;
			while(ptr < end && !isspace(*ptr))
			{
				ss.put(*ptr);
				ptr++;
			}

			if(ptr >= end)
				return 0;

			_msg->mSource = ss.str();
		}

		while(ptr < end && isspace(*ptr))
			ptr++;
		if(ptr >= end)
			return 0;

		// Parse command
		{
			std::stringstream ss;
			while(ptr < end && !isspace(*ptr))
			{
				ss.put(*ptr);
				ptr++;
			}

			_msg->mCommand = ss.str();
		}

		// Parse parameters
		_msg->mParameters.clear();
		while(ptr < end)
		{
			while(ptr < end && isspace(*ptr))
				ptr++;
			if(ptr >= end)
				break;

			std::string p;
			if(*ptr == ':')
			{
				ptr++;
				_msg->mParameters.push_back(std::string(ptr, end));
				ptr = end;
			}
			else
			{
				std::stringstream ss;
				while(ptr < end && !isspace(*ptr))
				{
					ss.put(*ptr);
					ptr++;
				}

				_msg->mParameters.push_back(ss.str());
			}
		}

		return done;
	}

	irc_message irc_message::parse(std::vector<char> &_data)
	{
		irc_message msg;
		size_t cnt = parse(&_data[0], _data.size(), &msg);
		if(cnt > 0)
			_data.erase(_data.begin(), _data.begin() + cnt);

		return msg;
	}
	
	std::ostream &irc_message::write(std::ostream &_str) const
	{
		if(!mSource.empty())
			_str << ":" << mSource << ' ';

		_str << mCommand;

		for(auto it = mParameters.begin();
			it != mParameters.end(); it++)
		{
			std::string const& str = *it;
			
			auto next = it;
			next++;

			if(str.find(' ') != std::string::npos
				|| (!str.empty() && str[0] == ':'))
			{
				_str << " :" << str;

				while(next != mParameters.end())
				{
					_str << " " << *next;
					next++;
				}
				break;
			}
			else
				_str << ' ' << *it;
		}

		return _str << irc::endl;
	}

	void irc_message::write(bitstream *_str) const
	{
		_str->write(str());
	}

	std::string irc_message::str() const
	{
		std::stringstream ss;
		write(ss);
		return ss.str();
	}

	NETLIB_API irc_message::parameters_t &operator <<(irc_message::parameters_t &_l, std::string const& _str)
	{
		_l.push_back(_str);
		return _l;
	}

	namespace irc
	{
		NETLIB_API std::ostream &endl(std::ostream &_str)
		{
			return _str << "\r\n";
		}

		NETLIB_API irc_message notice(std::string const& _text, std::string const& _to)
		{
			irc_message::parameters_t params;
			if(_to.empty())
				params.push_back("*");
			else
				params.push_back(_to);
			params.push_back(_text);

			return irc_message("NOTICE", params);
		}

		NETLIB_API irc_message error(std::string const& _text, std::string const& _sender)
		{
			irc_message::parameters_t params;
			params.push_back(_text);

			return irc_message("ERROR", params, _sender);
		}

		NETLIB_API irc_message nick(std::string const& _nick, std::string const& _src)
		{
			irc_message::parameters_t params;
			params.push_back(_nick);

			return irc_message("NICK", params, _src);
		}
	}
}
