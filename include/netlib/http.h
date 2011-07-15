#include "bitstream.h"
#include <map>
#include <string>
#include <sstream>

namespace netlib
{
	class socket;

	namespace websocket
	{
		class websocket;
	}

	typedef std::map<std::string, std::string> http_headers;

	class NETLIB_API http_request
	{
	public:
		http_request(std::string const& _method, std::string const& _path,
					std::string const& _version, http_headers const& _hdr, std::string const& _strm);
		~http_request();

		std::string const& method() const;
		std::string const& path() const;
		std::string const& version() const;

		const http_headers &headers() const;
		std::string header(std::string const&) const;
		const std::stringstream &data() const;

		bool is_websocket_request() const;

	protected:
		std::string mMethod;
		std::string mPath;
		std::string mVersion;
		http_headers mHeaders;
		std::stringstream mData;
	};

	class NETLIB_API http_response: public bitstream
	{
	public:
		using bitstream::read;
		using bitstream::read_block;
		using bitstream::write;
		using bitstream::write_block;

		http_response(socket &_sock, int _response=200,
			std::string const& _message="OK", std::string const& _version="1.1");
		~http_response();

		socket &socket();

		http_headers &headers();
		std::string header(std::string const&) const;
		void set_header(std::string const&, std::string const&);
		void clear_header(std::string const&);

		int response() const;
		std::string const& message() const;
		void set_response(int _response, std::string const& _message);

		std::string const& version() const;
		void set_version(std::string const& _v);

		// Set default parameters based on request.
		void setup(http_request const& _req);

		virtual size_t read(void *_buffer, size_t _amt);
		virtual size_t write(const void *_buffer, size_t _amt);
		virtual void flush();

		// Send the headers now, no Content-Length will be
		// appended, and you can transfer information in a 
		// streaming fashion.
		void send_headers();

		websocket::websocket accept_websocket(http_request const& _req, std::string const& _prot);

	protected:
		netlib::socket &mSocket;
		bool mStarted;
		int mResponse;
		std::string mMessage;
		std::string mVersion;
		http_headers mHeaders;
		std::stringstream mData;
	};

	class NETLIB_API http_handler
	{
	public:
		http_handler();
		
		virtual bool handle_request(socket &_sock);
		virtual bool handle_request(http_request const& _req, http_response &_resp);
		
		virtual bool handle_socket(socket &_sock);
	};
}
