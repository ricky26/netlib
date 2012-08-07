#include <netlib/netlib.h>
#include <netlib/init.h>
#include <netlib/uthread.h>

namespace netlib
{
	static struct netlib_initializer
	{
		inline netlib_initializer()
		{
			netlib::init();
		}
	} gInitializer;

	NETLIB_API void spawn_io_threads(int _count)
	{
		for(int i = 0; i < _count; i++)
		{
			thread::create([](){
				while(idle(uthread::schedule()), true);
			});
		}
	}
}