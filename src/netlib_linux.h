#include <sys/epoll.h>
#include <netlib/uthread.h>
#include <list>

namespace netlib
{
	extern int gEPollFd;

	struct aio_struct
	{
		typedef std::list<uthread::handle_t> list_t;

		~aio_struct();

		void make_nonblocking(int _fd);

		void begin_in();
		void end_in();

		void begin_out();
		void end_out();

		list_t in;
		list_t out;
	};
}

