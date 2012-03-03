#include "netlib/http.h"
#include "netlib/socket.h"
#include "netlib/terminal.h"
#include "netlib/md5.h"
#include "netlib/sha1.h"
#include "netlib/base64.h"
#include "netlib/websocket.h"
#include <string.h>

#ifdef _WIN32
#define strcasecmp stricmp
#endif

namespace netlib
{
	using namespace terminal;
	namespace ws=netlib::websocket;

	//
	// http_request
	//

	http_request::http_request(std::string const& _method, std::string const& _path,
					std::string const& _version, http_headers const& _hdr, std::string const& _strm)
		: mHeaders(_hdr.begin(), _hdr.end()), mData(_strm, std::ios::in), mMethod(_method),
			mPath(_path), mVersion(_version)
	{
	}

	http_request::~http_request()
	{
	}

	std::string const& http_request::method() const
	{
		return mMethod;
	}

	std::string const& http_request::path() const
	{
		return mPath;
	}

	std::string const& http_request::version() const
	{
		return mVersion;
	}

	const http_headers &http_request::headers() const
	{
		return mHeaders;
	}
	
	std::string http_request::header(std::string const& _nm) const
	{
		http_headers::const_iterator it;
		if((it = mHeaders.find(_nm)) != mHeaders.end())
			return it->second;

		return std::string();
	}

	const std::stringstream &http_request::data() const
	{
		return mData;
	}

	
	bool http_request::is_websocket_request() const
	{
		return header("Connection").find("Upgrade") != std::string::npos
			&& (strcasecmp(header("Upgrade").data(), "WebSocket") == 0);
	}

	//
	// http_response
	//

	http_response::http_response(netlib::socket &_sock, int _response,
			std::string const& _message, std::string const& _version)
			: mSocket(_sock), mStarted(false), mResponse(_response),
				mMessage(_message), mVersion(_version)
	{
	}

	http_response::~http_response()
	{
	}

	socket &http_response::socket()
	{
		return mSocket;
	}

	http_headers &http_response::headers()
	{
		return mHeaders;
	}

	std::string http_response::header(std::string const& _nm) const
	{
		http_headers::const_iterator it;
		if((it = mHeaders.find(_nm)) != mHeaders.end())
			return it->second;

		return std::string();
	}

	void http_response::set_header(std::string const& _nm, std::string const& _val)
	{
		mHeaders[_nm] = _val;
	}

	void http_response::clear_header(std::string const& _nm)
	{
		http_headers::iterator it;
		if((it = mHeaders.find(_nm)) != mHeaders.end())
			mHeaders.erase(it);
	}

	int http_response::response() const
	{
		return mResponse;
	}

	std::string const& http_response::message() const
	{
		return mMessage;
	}

	void http_response::set_response(int _response, std::string const& _message)
	{
		mResponse = _response;
		mMessage = _message;
	}

	std::string const& http_response::version() const
	{
		return mVersion;
	}

	void http_response::set_version(std::string const& _v)
	{
		mVersion = _v;
	}

	void http_response::setup(http_request const& _req)
	{
		set_version(_req.version());
		set_header("Host", _req.header("Host"));

		// TODO: Set Host: and stuff. : < -- Ricky26
	}

	size_t http_response::read(void *_buffer, size_t _amt)
	{
		// You can't read from the response. D: <

		return 0;
	}

	size_t http_response::write(const void *_buffer, size_t _amt)
	{
		if(!mStarted)
		{
			mData.write((char*)_buffer, _amt);
			return _amt;
		}

		return mSocket.write(_buffer, _amt);
	}

	void http_response::flush()
	{
		if(!mStarted)
		{
			std::stringstream cl;
			cl << mData.str().length();
			mHeaders["Content-Length"] = cl.str();

			send_headers();
		}
		
		std::string str = mData.str();
		if(str.length())
		{
			mSocket.write(str.data(), str.length());
			mData.str("");
		}
	}

	void http_response::send_headers()
	{
		if(!mStarted)
		{
			mStarted = true;

			std::stringstream headers;
			headers << "HTTP/" << version() << " "
				<< response() << " " << message() << crlf;

			for(http_headers::const_iterator it = mHeaders.begin();
				it != mHeaders.end(); it++)
			{
				headers << it->first << ": " << it->second << crlf;
			}

			headers << crlf;
			mSocket.write(headers.str().data(), headers.str().size());
		}
	}	

	static int parse_websocket_key(std::string const& _str)
	{
		size_t val = 0;
		size_t div = 0;
		for(std::string::const_iterator it = _str.begin();
			it != _str.end(); it++)
		{
			char c = *it;
			if(isdigit(c))
			{
				val = (val*10) + (c-'0');
			}
			else if(isspace(c))
				div++;
		}
		return (int)(val/div);
	}

	websocket_constructor_t http_response::accept_websocket(http_request const& _req, std::string const& _protocol)
	{
		std::string key1 = _req.header("Sec-WebSocket-Key1");
		if(!key1.empty())
		{
			// Old websocket Auth

			std::string key2 = _req.header("Sec-WebSocket-Key2");

			int v1 = parse_websocket_key(key1);
			int v2 = parse_websocket_key(key2);

			std::stringstream ss;
			ss << v1 << v2 << _req.data().str();

			md5 hash;
			hash.begin();
			hash.crypto::hash::update(ss.str());
			std::string resp = hash.compute();

			set_response(101, "WebSocket Protocol Handshake");
			set_header("Upgrade", "WebSocket");
			set_header("Connection", "Upgrade");
			set_header("Sec-WebSocket-Origin", _req.header("Origin"));
			set_header("Sec-WebSocket-Location", "ws://" + _req.header("Host") + _req.path());
			set_header("Sec-WebSocket-Protocol", _protocol);
			send_headers();

			write(resp.data(), resp.size());
			flush();

			return &mSocket;
		}
		else
		{
			key1 = _req.header("Sec-WebSocket-Key") + std::string("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

			sha1 hash;
			hash.begin();
			hash.update(key1);

			base64 enc;
			key1 = enc.encode(hash.compute());

			set_response(101, "Switching Protocols");
			clear_header("Host");
			set_header("Upgrade", "websocket");
			set_header("Connection", "Upgrade");
			set_header("Sec-WebSocket-Accept", key1);
			set_header("Sec-WebSocket-Version", "8");
			set_header("Sec-WebSocket-Origin", _req.header("Origin"));
			set_header("Sec-WebSocket-Location", "ws://" + _req.header("Host") + _req.path());
			set_header("Sec-WebSocket-Protocol", _protocol);
			send_headers();
			flush();

			return &mSocket;
		}

		return nullptr;
	}

	//
	// http_handler
	//

	http_handler::http_handler()
	{
	}

	bool http_handler::handle_request(socket &_sock)
	{
		http_headers headers;
		std::string method, path, version, key;
		std::stringstream builder;

		enum
		{
			state_method,
			state_path,
			state_version,
			state_key,
			state_value,
			state_data
		} state = state_method;

		bool was_return = false;
		char buffer[1024];
		size_t done = 0;
		size_t sz;
		while(!done && (sz = _sock.read(buffer, sizeof(buffer)), sz > 0))
		{
			for(size_t i = 0; i < sz && !done; i++)
			{
				char c = buffer[i];

				if(was_return && c == '\n')
				{
					// Got return carriage

					if(state == state_version)
					{
						std::string str = builder.str();
						builder.str("");
						if(str.length() < 5
							|| str.substr(0, 5) != "HTTP/")
						{
							//throw std::exception("Nonsense HTTP request.");
							return false;
						}

						version = str.substr(5);
						state = state_key;
					}
					else if(state == state_value)
					{
						headers[key] = builder.str();
						builder.str("");

						state = state_key;
					}
					else if(state == state_key)
					{
						done = i+1;
						break;
					}
				}
				else if(c == '\r')
				{
					was_return = true;
					continue;
				}
				else if(((c & 0x80) == 0) && isspace(c))
				{
					// Got space

					if(state == state_method)
					{
						method = builder.str();
						builder.str("");
						state = state_path;
					}
					else if(state == state_path)
					{
						path = builder.str();
						builder.str("");
						state = state_version;
					}
					else if(state > state_version && !builder.str().empty())
						builder.put(c);
				}
				else if(state == state_key && c == ':')
				{
					// Got :
					if(state == state_key)
					{
						key = builder.str();
						builder.str("");

						state = state_value;
					}
					else
						builder.put(c);
				}
				else
				{
					// Got a 'normal' character.
					builder.put(c);
				}

				was_return = false;
			}
		}

		builder.clear();
		size_t content_done = sz-done;

		if(done < sz)
			builder.write(buffer+done, content_done);

		size_t content_max = 0;
		if(headers.find("Content-Length") != headers.end())
		{
			std::string cl = headers.at("Content-Length");
			std::stringstream clss(cl);
			clss >> content_max;
		}

		while(content_done < content_max)
		{
			sz = _sock.read(buffer, sizeof(buffer));
			if(sz == 0)
				break;

			builder.write((char*)buffer, sz);
		}

		http_request req(method, path, version, headers, builder.str());

		http_response resp(_sock);
		resp.setup(req);
		bool ret = handle_request(req, resp);

		resp.flush();

		bool close = false;
		http_headers::const_iterator it = headers.find("Connection");
		if(it != headers.end() && it->second == "close")
			close = true;
		
		if(close || !ret)
			_sock.close();

		return ret;
	}

	bool http_handler::handle_request(http_request const& _req, http_response &_resp)
	{
		return false;
	}
		
	bool http_handler::handle_socket(socket &_sock)
	{
		if(!_sock.valid())
			return false;

		while(_sock.valid())
		{
			if(!handle_request(_sock))
				return false;
		}

		return true;
	}
}
