#include "netlib/netlib.h"
#include "netlib/thread.h"
#include "netlib/uthread.h"
#include "netlib/socket.h"
#include "netlib/pipe.h"
#include "netlib/file.h"
#include "netlib_linux.h"
#include <fcntl.h>

#define MAX_EVENTS 16

namespace netlib
{	
	int gEPollFd = -1;
	static bool gIsDone;
	static int gRetVal;
	
	aio_struct::~aio_struct()
	{
		for(list_t::const_iterator it = in.begin();
				it != in.end(); it++)
			(*it)->resume();

		for(list_t::const_iterator it = out.begin();
				it != out.end(); it++)
			(*it)->resume();
	}

	void aio_struct::make_nonblocking(int _fd)
	{
		int flags = fcntl(_fd, F_GETFL, 0);
		if(flags == -1)
			flags = 0;

		flags |= O_NONBLOCK;
		fcntl(_fd, F_SETFL, flags);

		epoll_event event;
		event.data.ptr = this;
		event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP
			| EPOLLHUP | EPOLLET;
		epoll_ctl(gEPollFd, EPOLL_CTL_ADD, _fd, &event);
	}

	void aio_struct::begin_in()
	{
		bool susp = !in.empty();
		in.push_back(uthread::current());

		if(susp)
			uthread::suspend();
	}

	void aio_struct::end_in()
	{
		in.pop_front();
		if(!in.empty())
			in.front()->resume();
	}

	void aio_struct::begin_out()
	{
		bool susp = !out.empty();
		out.push_back(uthread::current());

		if(susp)
			uthread::suspend();
	}

	void aio_struct::end_out()
	{
		out.pop_front();
		if(!out.empty())
			out.front()->resume();
	}

	NETLIB_API bool init()
	{
		gIsDone = false;
		gRetVal = 0;

		gEPollFd = epoll_create(10); // Note: 10 is ignored.
		if(gEPollFd == -1)
			return false;

		if(!thread::init())
			return false;

		if(!uthread::init())
			return false;

		if(!socket::init())
			return false;

		if(!pipe::init())
			return false;

		if(!file::init())
			return false;

		return true;
	}

	NETLIB_API void exit(int _val)
	{
		if(!gIsDone)
		{
			gRetVal = _val;
			gIsDone = true;

			file::shutdown();
			pipe::shutdown();
			socket::shutdown();
			uthread::shutdown();
			thread::shutdown();

			if(gEPollFd != -1)
			{
				close(gEPollFd);
				gEPollFd = -1;
			}
		}
	}

	NETLIB_API int exit_value()
	{
		return gRetVal;
	}

	NETLIB_API bool think()
	{
		if(gIsDone)
			return false;

		epoll_event events[MAX_EVENTS];
		int num = epoll_wait(gEPollFd, events, MAX_EVENTS, 0);
		for(int i = 0; i < num; i++)
		{
			aio_struct *aio = (aio_struct*)events[i].data.ptr;
			
			if(events[i].events & EPOLLIN)
			{
				if(!aio->in.empty())
					aio->in.front().get()->resume();
			}

			if(events[i].events & EPOLLOUT)
			{
				if(!aio->out.empty())
					aio->out.front().get()->resume();
			}
		}

		return true;
	}

	NETLIB_API int run_main_loop()
	{
		while(think())
			uthread::schedule();

		return exit_value();
	}
}
