#include "netlib.h"
#include <string>
#include <vector>
#include <map>

#pragma once

namespace
{
	class stringstream;
}

namespace netlib
{
	namespace json
	{
		enum type
		{
			integer,
			decimal,
			string,
			list,
			dict,
			boolean,
			null,

			invalid
		};

		class NETLIB_API object
		{
		public:
			typedef std::string string_t;
			typedef std::vector<object> list_t;
			typedef std::map<string_t, object> dict_t;

			object();
			object(int);
			object(double);
			object(std::string const&);
			object(const char *);
			object(list_t const&);
			object(dict_t const&);
			object(bool _b);
			object(type);

			object(object const&);
			~object();

			bool valid() const;

			size_t size() const;
			bool has_key(std::string const& _str) const;

			object item(size_t _idx) const;
			object item(std::string const& _nm) const;
			void append(object const& _item);

			void set(std::string const& _key, object const& _val);

			int integer() const;
			double decimal() const;
			string_t string() const;
			const list_t &list() const;
			const dict_t dict() const;
			bool boolean() const;

			object operator [](size_t _idx) const;
			object operator [](std::string const& _nm) const;

			std::string stringify() const;
			void stringify(std::stringstream &) const;

			static object parse(std::string const& _str);
			static object parse(std::stringstream &_ss);

		protected:
			type mType;
			int mInteger;
			double mDecimal;
			string_t mString;
			list_t mList;
			dict_t mDict;
			bool mBoolean;
		};
	}
}
