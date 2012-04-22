#include "netlib/data.h"
#include <cstdlib>
#include <new>

namespace netlib
{
	data::handle_t data::copy(void *_data, size_t _sz)
	{
		void *p = std::malloc(_sz);
		if(!p)
			throw std::bad_alloc();

		std::memcpy(p, _data, _sz);

		return new data(p, _sz);
	}
}