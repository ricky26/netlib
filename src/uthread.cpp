#include <netlib/uthread.h>
#include <algorithm>

namespace netlib
{
	//
	// uthread
	//

	struct thread_state
	{
		typedef std::pair<uthread::handle_t, uint64_t> sleeper_t;
		typedef std::list<sleeper_t> sleepers_t;

		sleepers_t sleepers;
	};

	static NETLIB_THREAD thread_state *g_thread_state;

	void uthread::suspend()
	{
		// If we hold a handle over uthread::exit, we won't ever deallocate it! harhar!
		uthread *thr = current().get();
		thr->mSuspended = true;
		if(!schedule())
		{
			throw std::exception("Can't suspend last uthread!");
			thr->mSuspended = false;
		}
	}

	void uthread::resume()
	{
		swap(this);
	}

	void uthread::run(run_t const& _fn)
	{
		mRun = _fn;
		swap(this);
	}

	void uthread::sleep(int _ms)
	{
		thread_state::sleepers_t &sleepers = g_thread_state->sleepers;
		uthread::handle_t cur = current();

		uint64_t tm = time() + _ms*1000;
		thread_state::sleeper_t sleeper(cur, tm);

		auto it = sleepers.begin();
		for(;it != sleepers.end(); it++)
		{
			if(it->second > tm)
			{
				sleepers.insert(it, sleeper);
				break;
			}
		}
		
		if(it == sleepers.end())
			sleepers.push_back(sleeper);

		uthread::suspend();
	}
	
	void uthread::wake_sleepers()
	{
		uint64_t tm = time();
		thread_state::sleepers_t &sleepers = g_thread_state->sleepers;

		while(!sleepers.empty()
			&& sleepers.front().second < tm)
		{
			handle_t thr = sleepers.front().first;
			sleepers.pop_front();
			swap(thr.get());
		}
	}
	
	bool uthread::deadline(uint64_t &_tm)
	{
		thread_state::sleepers_t &sleepers = g_thread_state->sleepers;
		if(sleepers.empty())
			return false;

		_tm = sleepers.front().second;
		return true;
	}

	void uthread::enter_thread_common()
	{
		g_thread_state = new thread_state;
	}

	void uthread::exit_thread_common()
	{
		delete g_thread_state;
	}

	//
	// channel
	//

	channel::channel(): mStatus(0)
	{
	}

	channel::~channel()
	{
		mStatus = 0;
		std::for_each(mValues.begin(), mValues.end(),
			[](element &_el) {
				_el.thread->raise(std::exception());
		});
	}

	void channel::write_ptr(void *_val)
	{
		mLock.lock();
		if(mStatus >= 0)
		{
			// write was first
			element el = { uthread::current(), _val, nullptr };
			mValues.push_back(el);
			mStatus++;
			mLock.unlock();
			uthread::suspend();
		}
		else
		{
			// write was second
			element el = mValues.front();
			mValues.pop_front();
			mStatus--;
			mLock.unlock();

			el.fn(el.param, _val);
			uthread::swap(el.thread.get());
		}
	}

	void channel::read_ptr(read_fn_t _fn, void *_param)
	{
		mLock.lock();
		if(mStatus <= 0)
		{
			// read was first

			element el = { uthread::current(), _param, _fn };
			mValues.push_back(el);
			mStatus--;
			mLock.unlock();
			uthread::suspend();
		}
		else
		{
			// read was second
			element el = mValues.front();
			mValues.pop_front();
			mStatus--;
			mLock.unlock();

			_fn(_param, el.param);
			uthread::swap(el.thread.get());
		}
	}
}
