#include "event_batch.h"


namespace wlf::evt {

	EventBatch::EventBatch(const EventHandle& subscription) noexcept
		: _subscription(subscription)
	{
	}


	EventBatch::~EventBatch()
	{
		clear_unconsumed_events();
	}


	EventBatch::FetchResult EventBatch::fetch(DWORD timeout) noexcept
	{
		// Prevent handle leaks from previously unconsumed events
		clear_unconsumed_events();

		// Attempt to fetch the next batch
		const DWORD events_size = static_cast<DWORD>(_events.size());
		if (::EvtNext(_subscription.get(), events_size, _events.data(), timeout, 0, &_count)) {
			return FetchResult::Continue;
		}

		// Map the error if the fetch failed
		switch (::GetLastError()) {
		case ERROR_NO_MORE_ITEMS: return FetchResult::Completed;
		case ERROR_CANCELLED:     return FetchResult::Cancelled;
		default:                  return FetchResult::Failed;
		}
	}


	EventHandle EventBatch::next() noexcept
	{
		if (_next >= _count)
			return EventHandle();

		// Extract the handle, nullify the array entry to prevent the destructor 
		// from closing it, and transfer ownership to the caller.
		EventHandle event{ _events[_next] };
		_events[_next++] = nullptr;

		return event;
	}


	void EventBatch::clear_unconsumed_events() noexcept
	{
		// Close any handles that were fetched but never consumed via next()
		for (DWORD j = _next; j < _count; ++j) {
			if (_events[j] != nullptr) {
				::EvtClose(_events[j]);
				_events[j] = nullptr;
			}
		}

		_count = 0;
		_next = 0;
	}

}