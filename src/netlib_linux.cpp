#include "netlib/netlib.h"
#include "netlib/thread.h"
#include "netlib/uthread.h"
#include "netlib/socket.h"
#include "netlib/pipe.h"
#include "netlib/file.h"
#include "netlib/exception.h"
#include "netlib_linux.h"
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_EVENTS 16

namespace netlib
{	
	int gEPollFd = -1;
	static bool gIsDone;
	static int gRetVal;

	static bool at_exit_done()
	{
		static bool gDone = false;
		bool ret = gDone;
		gDone = true;
		return !ret;
	}

	static bool netlib_setup_done(bool _init=true)
	{
		static bool gDone = false;
		bool ret = gDone;
		gDone = _init;

		return ret ^ _init;
	}

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
		uthread::handle_t thread =
			uthread::current();

		bool susp = !in.empty();
		in.push_back(thread);
		
		if(susp)
			thread->suspend([](){});
	}

	void aio_struct::end_in()
	{
		in.pop_front();
		if(!in.empty())
			in.front()->resume();
	}

	void aio_struct::begin_out()
	{
		uthread::handle_t thread =
			uthread::current();
		bool susp = !out.empty();
		out.push_back(uthread::current());

		if(susp)
			thread->suspend([](){});
	}

	void aio_struct::end_out()
	{
		out.pop_front();
		if(!out.empty())
			out.front()->resume();
	}

	NETLIB_API void shutdown()
	{
		if(netlib_setup_done(false))
		{
			file::shutdown();
			pipe::shutdown();
			socket::shutdown();
			uthread::shutdown();
			thread::shutdown();

			if(gEPollFd != -1)
			{
				::close(gEPollFd);
				gEPollFd = -1;
			}
		}
	}

	NETLIB_API void init()
	{
		if(netlib_setup_done())
		{
			if(at_exit_done())
				atexit(shutdown);
			
			gEPollFd = epoll_create(10); // Note: 10 is ignored.

			thread::init();
			uthread::init();
			socket::init();
			pipe::init();
			file::init();

			idle(false);
		}
	}

	NETLIB_API void exit(int _val)
	{
	}

	NETLIB_API int exit_value()
	{
		return gRetVal;
	}

	NETLIB_API bool process_io_single(int _timeout)
	{
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

		return num > 0;
	}

	NETLIB_API void process_io(bool _can_block)
	{
		int timeout = 0;
		if(_can_block)
		{
			uint64_t dl, t=time();
			if(!uthread::deadline(dl)) 
				timeout = -1;
			else if(dl > t)
				timeout = (int)((dl-t)/1000);
		}

		process_io_single(timeout);
	}

	NETLIB_API void idle(bool _can_block)
	{
		process_io(_can_block);
		thread::sleep(0);
	}

	NETLIB_API void idle_slave(bool _can_block)
	{
		process_io(_can_block);
	}

	NETLIB_API int run_main_loop()
	{
		try
		{
			for(;;) { idle(!uthread::schedule()); }
		}
		catch(quit_exception const& _e)
		{
			return _e.value();
		}
	}
	
	NETLIB_API uint64_t time()
	{
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);

		return ts.tv_nsec + 1000000*(uint64_t)ts.tv_sec;
	}
}
