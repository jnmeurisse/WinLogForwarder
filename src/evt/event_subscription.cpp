#include "event_subscription.h"
#include <stdexcept>
#include <utility>

#include "evt/event_batch.h"


namespace wlf::evt {


	EventSubscription::EventSubscription(const Options& options) noexcept
		: _signal(true)
		, _subscription(_signal, options.channel, options.query)
		, _processor_context(options.processing_context)
	{
		if (!_subscription) {
			_subscription_error = _subscription.error();
			_cancelled = true;
		}
	}


	EventSubscription::DrainResult EventSubscription::drain(const EventHandlerCallback& handler) noexcept
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
		_stats.fetch_count++;

		// Handle the result of the fetch operation.
		switch (fetch_result) {
		case EventBatch::FetchResult::Continue:
			// Process each event in the batch using the provided handler.
			// Continue processing even if some events fail to be handled.
			// The handler could return false if it fails to format the event
			// log or if it encounters an error during processing.
			while (auto event = batch.next()) {
				_stats.event_count++;
				if (!handler(_processor_context, event))
					_stats.event_failed_count++;
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
