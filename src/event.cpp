#include "netlib\event.h"

namespace netlib
{
	//
	// event
	//

	event::event(void *_sender)
		: mAccepted(false), mSender(_sender)
	{
	}

	event::~event()
	{
	}

	void event::accept()
	{
		mAccepted = true;
	}

	//
	// event_handler
	//

	event_handler::event_handler()
	{
	}

	event_handler::~event_handler()
	{
	}

	//
	// function_event_handler
	//

	function_event_handler::function_event_handler()
	{
	}

	function_event_handler::function_event_handler(fn_t const& _fn)
		: mFunction(_fn)
	{
	}

	void function_event_handler::set(fn_t const& _fn)
	{
		mFunction = _fn;
	}

	void function_event_handler::handle(event *_evt)
	{
		if(mFunction)
			mFunction(_evt);
	}

	//
	// delegate
	//

	delegate::delegate()
	{
	}

	delegate::~delegate()
	{
	}

	void delegate::clear()
	{
		mHandlers.clear();
	}

	void delegate::add_handler(event_handler *_h)
	{
		mHandlers.insert(_h);
	}

	void delegate::remove_handler(event_handler *_h)
	{
		auto it = mHandlers.find(_h);
		if(it != mHandlers.end())
			mHandlers.erase(it);
	}

	void delegate::notify(event *_evt)
	{
		set_t handlers(mHandlers.begin(), mHandlers.end());
		for(auto it = handlers.begin();
			it != handlers.end(); it++)
		{
			if(_evt->accepted())
				break;

			(*it)->handle(_evt);
		}
	}
}
