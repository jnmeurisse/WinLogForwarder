#include "event_batch.h"

#include <span>


namespace wlf::evt {

	EventBatch::EventBatch(const EventSubscriptionHandle& subscription) noexcept
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
		if (_subscription.next(std::span<::EVT_HANDLE>(_events), timeout, _count)) {
			return FetchResult::Continue;
		}

		// Map the error if the fetch failed
		switch (::GetLastError()) {
		case ERROR_NO_MORE_ITEMS: return FetchResult::Completed;
		case ERROR_CANCELLED:     return FetchResult::Cancelled;
		default:                  return FetchResult::Failed;
		}
	}


	EventLogHandle EventBatch::next() noexcept
	{
		if (_next >= _count)
			return EventLogHandle();

		// Extract the handle, nullify the array entry to prevent the destructor 
		// from closing it, and transfer ownership to the caller.
		EventLogHandle event{ _events[_next] };
		_events[_next++] = nullptr;

		return event;
	}


	void EventBatch::clear_unconsumed_events() noexcept
	{
		// Close any handles that were fetched but never consumed via next()
		for (DWORD i = _next; i < _count; ++i) {
			if (_events[i] != nullptr) {
				::EvtClose(_events[i]);
				_events[i] = nullptr;
			}
		}

		_count = 0;
		_next = 0;
	}

}