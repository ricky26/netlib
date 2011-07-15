#include "netlib/json.h"
#include <iterator>
#include <sstream>

namespace netlib
{
	namespace json
	{
		object::object(): mType(invalid)
		{
		}

		object::object(int _i): mType(json::integer), mInteger(_i)
		{
		}

		object::object(double _d): mType(json::decimal), mDecimal(_d)
		{
		}

		object::object(std::string const& _s): mType(json::string), mString(_s)
		{
		}

		object::object(const char *_s): mType(json::string), mString(_s)
		{
		}

		object::object(list_t const& _l): mType(json::list), mList(_l)
		{
		}

		object::object(dict_t const& _d): mType(json::dict), mDict(_d)
		{
		}

		object::object(bool _b): mType(json::boolean), mBoolean(_b)
		{
		}

		object::object(type _t): mType(_t)
		{
		}

		object::object(object const& _b): mType(_b.mType)
		{
			switch(mType)
			{
			case json::integer:
				mInteger = _b.mInteger;
				break;

			case json::decimal:
				mDecimal = _b.mDecimal;
				break;

			case json::string:
				mString = _b.mString;
				break;

			case json::list:
				mList = _b.mList;
				break;

			case json::dict:
				mDict = _b.mDict;
				break;
			}
		}

		object::~object()
		{
		}

		bool object::valid() const
		{
			return mType != invalid;
		}

		size_t object::size() const
		{
			return mList.size();
		}

		bool object::has_key(std::string const& _str) const
		{
			return mDict.find(_str) != mDict.end();
		}

		object object::item(size_t _idx) const
		{
			return mList.at(_idx);
		}

		object object::item(std::string const& _nm) const
		{
			dict_t::const_iterator it = mDict.find(_nm);
			if(it == mDict.end())
				return object();

			return it->second;
		}

		void object::append(object const& _item)
		{
			mList.push_back(_item);
		}

		void object::set(std::string const& _key, object const& _val)
		{
			mDict[_key] = _val;
		}

		int object::integer() const
		{
			return mInteger;
		}

		double object::decimal() const
		{
			return mDecimal;
		}

		object::string_t object::string() const
		{
			return mString;
		}

		const object::list_t &object::list() const
		{
			return mList;
		}

		const object::dict_t object::dict() const
		{
			return mDict;
		}

		bool object::boolean() const
		{
			return mBoolean;
		}

		object object::operator [](size_t _idx) const
		{
			return item(_idx);
		}

		object object::operator [](std::string const& _nm) const
		{
			return item(_nm);
		}

		std::string object::stringify() const
		{
			std::stringstream ss;
			stringify(ss);
			return ss.str();
		}

		static void escape(std::stringstream &_ss, std::string const& _str)
		{
			_ss << '"';
			for(std::string::const_iterator it = _str.begin();
				it != _str.end(); it++)
			{
				char c = *it;
				switch(c)
				{
					case '\\':
					case '"':
						_ss.put('\\');

					default:
						_ss.put(c);
						break;
				}
			}
			_ss << '"';
		}

		void object::stringify(std::stringstream &_str) const
		{
			switch(mType)
			{
			case json::integer:
				_str << mInteger;
				break;

			case json::decimal:
				_str << mDecimal;
				break;

			case json::string:
				escape(_str, mString);
				break;

			case json::list:
				_str << "[ ";
				{
					for(list_t::const_iterator it = mList.begin();
						it != mList.end(); it++)
					{
						if(it != mList.begin())
							_str << ", ";

						it->stringify(_str);
					}
				}
				_str << ']';
				break;

			case json::dict:
				_str << "{ ";
				{
					for(dict_t::const_iterator it = mDict.begin();
						it != mDict.end(); it++)
					{
						if(it != mDict.begin())
							_str << ", ";

						escape(_str, it->first);
						_str << ": ";
						it->second.stringify(_str);
					}
				}
				_str << '}';
				break;
			}
		}

		static bool skip_ws(std::stringstream &_ss)
		{
			while((_ss.peek(), !_ss.eof())
				&& isspace(_ss.peek()))
				_ss.get();

			return !_ss.eof();
		}

		std::string parse_string(std::stringstream &_ss)
		{
			_ss.get(); // ignore "

			std::stringstream ret;
			char c;
			
			while((c = _ss.peek(), !_ss.eof()))
			{
				if(c == '\\')
					_ss.get();
				else if(c == '"')
				{
					_ss.get();
					break;
				}

				ret.put(_ss.get());
			}

			return ret.str();
		}

		object object::parse(std::string const& _str)
		{
			std::stringstream ss(_str);
			return parse(ss);
		}

		object object::parse(std::stringstream &_ss)
		{
			if(!skip_ws(_ss))
				return object();

			char fst = _ss.peek();
			if(isdigit(fst))
			{
				int num = fst-'0';
				_ss.get();
				
				while((fst = _ss.peek(), !_ss.eof())
					&& isdigit(fst))
					num = (num*10) + (fst-'0');

				// TODO: .

				return object(num);
			}
			else if(fst == '"')
			{
				return object(parse_string(_ss));
			}
			else if(fst == '[')
			{
				_ss.get();

				list_t ret;

				do
				{
					if(!skip_ws(_ss))
						return object();

					object o = parse(_ss);
					if(!o.valid())
						return o;

					ret.push_back(o);

					if(!skip_ws(_ss))
						return object();
				}
				while(_ss.peek() == ',' && _ss.get());
				
				if(!skip_ws(_ss))
					return object();

				if(_ss.peek() != ']')
					return object();

				_ss.get();

				return ret;
			}
			else if(fst == '{')
			{
				_ss.get();

				dict_t ret;

				do
				{
					if(!skip_ws(_ss))
						return object();

					std::string key = parse_string(_ss);

					if(!skip_ws(_ss))
						return object();

					if(_ss.get() != ':')
						return object();

					if(!skip_ws(_ss))
						return object();

					object o = parse(_ss);
					if(!o.valid())
						return o;

					ret[key] = o;

					if(!skip_ws(_ss))
						return object();
				}
				while(_ss.peek() == ',' && _ss.get());
				
				if(!skip_ws(_ss))
					return object();

				if(_ss.peek() != '}')
					return object();
				
				_ss.get();

				return ret;
			}
			else if(fst == 't')
			{
				_ss.get(); // t
				_ss.get(); // r
				_ss.get(); // u
				_ss.get(); // e
				return true;
			}
			else if(fst == 'f')
			{
				_ss.get(); // f
				_ss.get(); // a
				_ss.get(); // l
				_ss.get(); // s
				_ss.get(); // e
				return true;
			}
			else if(fst == 'n')
			{
				_ss.get(); // n
				_ss.get(); // u
				_ss.get(); // l
				_ss.get(); // l
				return true;
			}

			return object();
		}
	}
}
