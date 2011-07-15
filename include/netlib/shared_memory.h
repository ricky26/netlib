#include "netlib.h"
#include <string>

namespace netlib
{
	class NETLIB_API shared_memory
	{
	public:
		shared_memory();
		shared_memory(std::string const& _name, size_t _sz);
		virtual ~shared_memory();
		
		bool valid() const;
		size_t size() const;
		void close();

		bool open(std::string const& _name, size_t _sz);

		template<typename T>
		bool open(std::string const& _name)
		{
			return open(_name, sizeof(T));
		}

		bool create(std::string const& _name, size_t _sz);

		template<typename T>
		bool create(std::string const& _name)
		{
			return create(_name, sizeof(T));
		}

		void *pointer() const;

		template<typename T>
		T *pointer() const
		{
			if(size() != sizeof(T))
				return NULL;

			return static_cast<T*>(pointer());
		}

	protected:
		void *mInternal;
	};
};