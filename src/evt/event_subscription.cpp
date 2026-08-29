#include "event_subscription.h"

#include "evt/event_batch.h"


namespace wlf::evt {

	EventSubscription::EventSubscription(const std::string& id, 
        const std::wstring& channel, const std::wstring& query, const EventProcessContext& process_context)
		: _id(id)
        , _signal(true)
		, _subscription(EventSubscriptionHandle::create(_signal, channel, query))
		, _process_context(process_context)
	{
	}


	EventSubscription::DrainResult EventSubscription::drain(const EventHandlerCallback& handler)
	{
		_stats.drain_count++;

		if (_drained) {
			// If the subscription is already drained, check if the signal is set 
			// indicating that new events are available
			if (!_signal.is_set())
				return DrainResult::Completed;

			_signal.reset();
			_drained = false;
		}

		// Fetch the next batch of events from the subscription.
		EventBatch batch(_subscription);
		const auto fetch_result = batch.fetch(0);
        ++_stats.fetch_count;

		// Handle the result of the fetch operation.
		switch (fetch_result) {
		case EventBatch::FetchResult::Continue:
			// Process each event in the batch using the provided handler.
			// Continue processing even if some events fail to be handled.
			// The handler could return false if it fails to format the event
			// log or if it encounters an error during processing.
			while (auto event = batch.next()) {
                const ProcessResult result = handler(_process_context, event);
                if (result == ProcessResult::Filtered)
                    ++_stats.event_filtered_count;
                else if (result == ProcessResult::Failed)
                    _stats.event_failed_count++;
                else
                    ++_stats.event_count;
            }

			return DrainResult::Continue;

		case EventBatch::FetchResult::Completed:
			// No more events are available; mark the subscription as drained.
			_drained = true;
			return DrainResult::Completed;

		case EventBatch::FetchResult::Cancelled:
			// The subscription was cancelled; mark it as such and return Cancelled.
			return DrainResult::Cancelled;

		case EventBatch::FetchResult::Failed:
		default:
			// An error occurred while fetching events
			_stats.fetch_failed_count++;
			return DrainResult::Failed;
		}
	}


	bool EventSubscription::cancel() noexcept
	{
        if (_subscription && !_cancelled)
            _cancelled = _subscription.cancel();

		return _cancelled;
	}

}
