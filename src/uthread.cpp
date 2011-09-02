#include <netlib/uthread.h>

namespace netlib
{
	//
	// uthread
	//
	
	int uthread::protect()
	{
		handle_t thr = current();
		return ++thr->mProtection;
	}

	int uthread::unprotect()
	{
		handle_t thr = current();
		if(thr->mProtection == 0)
			return 0;

		return --thr->mProtection;
	}

	void uthread::schedule(netlib::scheduler *_sch)
	{
		_sch->add(this);
		_sch->schedule(this);
	}

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
		if(suspended())
		{
			scheduler()->schedule(this);

			handle_t cur = current();
			if(scheduler() == cur->scheduler())
				swap(this);
		}
	}

	void uthread::run(run_t const& _fn)
	{
		if(dead())
			return;

		uthread::handle_t cur = uthread::current();

		mSuspended = true;
		mRun = [_fn, cur]()
		{ 
			_fn(); swap(cur.get());
		};
		swap(this);
	}

	bool uthread::schedule()
	{
		uthread *cur = current().get();
		netlib::scheduler *sch = cur->scheduler();
		return sch->swap();
	}

	void uthread::sleep(int _ms)
	{
		uthread::handle_t cur = current();
		netlib::scheduler *sch = cur->scheduler();
		sch->sleep(cur, time() + (_ms*1000));
		uthread::suspend();
	}

	//
	// scheduler
	//

	scheduler::scheduler()
	{
	}

	scheduler::~scheduler()
	{
	}

	void scheduler::add(uthread::handle_t _thr)
	{
		netlib::scheduler *old = _thr->scheduler();
		if(old != this)
		{
			if(old)
				old->remove(_thr);

			_thr->mScheduler = this;
			_thr->mPosition = mScheduled.end();
			_thr->mSleepPosition = mSleepers.end();
		}
	}

	void scheduler::remove(uthread::handle_t _thr)
	{
		unschedule(_thr);
	}

	void scheduler::schedule(uthread::handle_t _thr)
	{
		if(_thr->mPosition != mScheduled.end())
			return;

		mLock.lock();
		mScheduled.push_back(_thr);
		auto it = mScheduled.end();
		it--;
		_thr->mPosition = it;
		mLock.unlock();
	}

	void scheduler::sleep(uthread::handle_t _thr, uint64_t _end)
	{
		if(_thr->mSleepPosition != mSleepers.end())
			return;

		mSleepLock.lock();
		uthread::sleep_pair_t to_insert(_thr, _end);
		auto it = mSleepers.begin();

		for(auto it = mSleepers.begin();
			it != mSleepers.end(); it++)
		{
			if(it->second > _end)
			{
				it--;
				mSleepers.insert(it, to_insert);
				it++;
				_thr->mSleepPosition = it;
				mSleepLock.unlock();
				return;
			}
		}

		mSleepers.push_back(to_insert);
		it = mSleepers.end();
		it--;
		_thr->mSleepPosition = it;
		mSleepLock.unlock();
	}

	void scheduler::unschedule(uthread::handle_t _thr)
	{
		mSleepLock.lock();
		if(_thr->mSleepPosition != mSleepers.end())
		{
			mSleepers.erase(_thr->mSleepPosition);
			_thr->mSleepPosition = mSleepers.end();
		}
		mSleepLock.unlock();

		mLock.lock();
		if(_thr->mPosition != mScheduled.end())
		{
			mScheduled.erase(_thr->mPosition);
			_thr->mPosition = mScheduled.end();
		}
		mLock.unlock();
	}

	bool scheduler::swap()
	{
		uthread *thr_p = NULL;
		uint64_t tm = time();

		mSleepLock.lock();
		if(!mSleepers.empty())
		{
			if(mSleepers.front().second < tm)
			{
				thr_p = mSleepers.front().first.get();
				thr_p->acquire();
				mSleepers.pop_front();
				thr_p->mSleepPosition = mSleepers.end();
				mSleepLock.unlock();

				return uthread::swap(thr_p);
			}
		}
		mSleepLock.unlock();

		do
		{
			mLock.lock();

			auto it = mScheduled.begin();
			if(it == mScheduled.end())
			{
				mLock.unlock();
				return false;
			}

			uthread::handle_t thr = *it;
			while(thr->thread() && (thr->thread() != thread::current()))
			{
				// TODO: This could eat CPU, we should fix this! D;! -- Ricky26
				it++;
				if(it == mScheduled.end())
				{
					mLock.unlock();
					return false;
				}

				thr = *it;
			}
		
			mLock.unlock();

			thr_p = thr.get();
			thr_p->acquire();
		}
		while(!uthread::swap(thr_p));

		return true;
	}

	static void scheduler_worker_thread(scheduler *sch)
	{
		uthread::current()->schedule(sch);
		netlib::run_main_loop();
	}

	thread::handle_t scheduler::create_worker_thread()
	{
		return thread::create(scheduler_worker_thread, this);
	}

	void scheduler::create_worker_threads(int _icnt)
	{
		for(int i = 0; i < _icnt; i++)
			create_worker_thread();
	}
}
