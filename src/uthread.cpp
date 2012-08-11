#include <netlib/uthread.h>
#include <netlib/linked_list.h>
#include <algorithm>

namespace netlib
{
	//
	// uthread
	//

	struct thread_state
	{
		typedef std::pair<uthread::handle_t, uint64_t> sleeper_t;
		typedef linked_list<sleeper_t> sleepers_t;

		sleepers_t sleepers, dead_sleepers, done_sleepers;
	};

	static NETLIB_THREAD thread_state *g_thread_state;
	
	void uthread::suspend(run_t const& _run)
	{
		mRun = _run;
		suspend();
	}
	
	void uthread::exit()
	{
		mDead = true;
		suspend();
	}

	void uthread::resume()
	{
		swap(this);
	}

	void uthread::run(run_t const& _fn)
	{
		if(mDead)
			return;

		mRun = _fn;
		swap(this);
	}

	void uthread::sleep(int _ms)
	{
		thread_state::sleepers_t &sleepers(g_thread_state->sleepers),
			&dead(g_thread_state->dead_sleepers);
		uthread::handle_t cur = current();

		uint64_t tm = time() + _ms*1000;
		thread_state::sleeper_t sleeper(cur, tm);

		auto it = sleepers.begin();
		for(;it != sleepers.end(); it++)
		{
			if(it->second > tm)
				break;
		}
		
		if(!dead.empty())
		{
			auto ss = dead.begin();
			ss->first = cur;
			ss->second = tm;

			it.splice(ss);
		}
		else
			sleepers.insert(it, sleeper);

		cur->suspend();
	}
	
	void uthread::nice()
	{
		sleep(0);
	}
	
	void uthread::wake_sleepers()
	{
		uint64_t tm = time();
		thread_state &t(*g_thread_state);

		while(!t.sleepers.empty())
		{
			auto it = t.sleepers.begin();
			if(it->second > tm)
				break;

			t.done_sleepers.begin().splice(it);
		}

		while(!t.done_sleepers.empty())
		{
			auto it = t.done_sleepers.begin();
			handle_t thr = it->first;
			t.dead_sleepers.begin().splice(it);

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

			uthread::current()->suspend();
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
			
			uthread::current()->suspend();
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
