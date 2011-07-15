#include "bitstream.h"
#include <string>
#include <vector>

namespace netlib
{
	class NETLIB_API irc_message
	{
	public:
		typedef std::vector<std::string> parameters_t;

		irc_message();
		irc_message(std::string const& _command,
			parameters_t const& _params=parameters_t(), std::string const &_src="");

		bool valid() const;

		std::string source() const;
		std::string command() const;

		const parameters_t &parameters() const;

		std::ostream &write(std::ostream &) const;
		void write(bitstream *) const;
		std::string str() const;
		
		static size_t parse(const char *_data, size_t _len,
			irc_message *_msg);
		static irc_message parse(std::vector<char> &_data);

	private:
		std::string mSource;
		std::string mCommand;
		parameters_t mParameters;
	};

	NETLIB_API irc_message::parameters_t &operator <<(irc_message::parameters_t&, std::string const&);

	namespace irc
	{
		NETLIB_API std::ostream &endl(std::ostream&);
		
		NETLIB_API irc_message notice(std::string const& _text);
		NETLIB_API irc_message nick(std::string const& _nick, std::string const& _src="");
	};
}