#include "netlib.h"
#include <functional>
#include <unordered_set>

#pragma once

namespace netlib
{
	class NETLIB_API event
	{
	public:
		event();
		virtual ~event();

		void accept();

		NETLIB_INLINE bool accepted() const { return mAccepted; }

	private:
		bool mAccepted;
	};

	class NETLIB_API event_handler
	{
	public:
		event_handler();
		virtual ~event_handler();

		virtual void handle(event *_evt) = 0;
	};

	class NETLIB_API function_event_handler: public event_handler
	{
	public:
		typedef std::function<void(event*)> fn_t;

		function_event_handler();
		function_event_handler(fn_t const& _fn);

		void set(fn_t const& _fn);
		NETLIB_INLINE fn_t const& function() const { return mFunction; }

		void handle(event *_evt) override;

	private:
		fn_t mFunction;
	};

	class NETLIB_API delegate
	{
	public:
		typedef std::unordered_set<event_handler*> set_t;

		delegate();
		virtual ~delegate();

		void clear();
		void add_handler(event_handler *_h);
		void remove_handler(event_handler *_h);

		void notify(event *_evt);
		
		NETLIB_INLINE void operator +=(event_handler *_fn) { add_handler(_fn); }
		NETLIB_INLINE void operator -=(event_handler *_fn) { remove_handler(_fn); }

	private:
		set_t mHandlers;
	};
}
