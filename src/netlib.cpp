#include <netlib/netlib.h>
#include <netlib/uthread.h>

namespace netlib
{
	NETLIB_API void spawn_io_threads(int _count)
	{
		for(int i = 0; i < _count; i++)
		{
			thread::create([](){
				run_main_loop();
			});
		}
	}
}