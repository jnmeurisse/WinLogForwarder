#include "event_collector.h"

#include <array>
#include <memory>


namespace wlf::evt {

    EventCollector::EventCollector(evt::EventQueue& queue, const EventCollectorConfig& config)
		: EventThread(queue)
		, _processor(config)
	{
	}


	void EventCollector::add_subscription(const EventSubscriptionConfig& config)
	{
		_subscriptions.push_back(std::make_unique<EventSubscription>(config));
	}


	unsigned int EventCollector::run()
	{
		auto event_handler = [this](const EventProcessor::Context& ctx, const EventLogHandle& event) {
			return this->handle_event(ctx, event);
		};

		// Main loop: drain all subscriptions, then wait for a signal or queue stop.
		while (!is_stopped()) {
			_watchdog = steady_clock::now();

			bool must_drain;
			do {
				must_drain = false;
		
				// Drain all subscriptions until they are all empty or cancelled.
				for (auto& subscription : _subscriptions) {
					// Skip cancelled subscriptions
					if (subscription->is_cancelled())
						continue;

					// Drain the subscription and check if more events are available.
					if (subscription->drain(event_handler) == EventSubscription::DrainResult::Continue)
						must_drain = true;
				}

			} while (must_drain);

			// Wait for a signal from any subscription or the queue stop event.
			::DWORD wait_result = wait_event(5000);
		}

		return 0;
	}


    void EventCollector::stop() noexcept
    {
        for (auto& subscription : _subscriptions)
            subscription->cancel();

        EventThread::stop();
    }


	ProcessResult EventCollector::handle_event(const EventProcessor::Context& ctx, const EventLogHandle& event)
	{
		std::unique_ptr<EventMessageBuilder> emb = std::make_unique<EventMessageBuilder>(ctx.max_message_size);

		// Process the event and push it into the message queue.
        ProcessResult result = _processor.process(ctx, event, *emb);
        if (result == ProcessResult::Success) {
            if (!push_message(std::move(emb)))
                result = ProcessResult::Failed;
        }

        return result;
	}


	DWORD EventCollector::wait_event(const DWORD timeout)
	{
		std::array<HANDLE, 16> handles{};
		DWORD handle_count = 0;

		handles[handle_count++] = stop_signal().get_handle();

		for (auto& subscription : _subscriptions) {
			if (!subscription->is_cancelled())
				handles[handle_count++] = subscription->get_signal().get_handle();
		}

		return ::WaitForMultipleObjects(handle_count, handles.data(), FALSE, timeout);
	}

}