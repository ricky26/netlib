#include <netlib/netlib.h>
#include <netlib/uthread.h>

namespace netlib
{
	NETLIB_API int run_main(std::function<void()> const& _fn)
	{
		uthread::create([_fn]() {
			try
			{
				_fn();
				exit(0);
			}
			catch(std::exception const&)
			{
				exit(0);
				throw;
			}
		});

		return run_main_loop();
	}
}