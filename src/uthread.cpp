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

	bool uthread::schedule()
	{
		netlib::scheduler *sch = current()->scheduler();
		return sch->swap();
	}

	void uthread::schedule(netlib::scheduler *_sch)
	{
		_sch->schedule(this);
	}

	void uthread::suspend()
	{
		handle_t thr = current();
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
			mSuspended = false;

			handle_t cur = current();
			if(scheduler() == cur->scheduler())
				swap(this);
		}
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

	void scheduler::schedule(uthread *_thr)
	{
		netlib::scheduler *old = _thr->scheduler();
		if(old != this)
		{
			if(old)
				old->unschedule(_thr);

			_thr->mScheduler = this;
			_thr->mPosition = mScheduled.end();
		}
		else if(_thr->mPosition != mScheduled.end())
			return;

		mLock.lock();
		mScheduled.push_back(_thr);
		auto it = mScheduled.end();
		it--;
		_thr->mPosition = it;
		mLock.unlock();
	}

	void scheduler::unschedule(uthread *_thr)
	{
		if(_thr->scheduler() != this
			|| _thr->mPosition == mScheduled.end())
			return;

		mLock.lock();
		mScheduled.erase(_thr->mPosition);
		_thr->mPosition = mScheduled.end();
		mLock.unlock();
	}

	bool scheduler::swap()
	{
		uthread::handle_t thr;
		do
		{
			mLock.lock();

			auto it = mScheduled.begin();
			if(it == mScheduled.end())
			{
				mLock.unlock();
				return false;
			}

			thr = *it;
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
		}
		while(!uthread::swap(thr));

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
