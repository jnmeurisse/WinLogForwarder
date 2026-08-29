#include "event_collector.h"

#include <array>
#include <memory>
#include "utl/exception.h"
#include "utl/string.h"


namespace wlf::evt {

    EventCollector::EventCollector(EventQueue& queue, const EventCollectorConfig& config)
		: EventThread(queue)
		, _processor(config.processor_config)
	{
	}


    void EventCollector::add_filter(const EventFilterConfig& config)
    {
    }


    void EventCollector::add_formatter(const EventFormatterConfig& config)
    {
        auto formatter = std::make_shared<RFC5424EventFormatter>(config);
        _formatters.try_emplace(config.id, formatter);
    }


    void EventCollector::add_subscription(const EventSubscriptionConfig& config)
    {
        // Check for duplicate subscription
        for (const auto& subscription : _subscriptions) {
            if (utl::iequal(subscription->id(), config.id))
                return;
        }

        _logger.info("Configure subscription %s", config.id.c_str());
        _logger.info("   channel=%S", config.channel.c_str());
        if (config.event_id_filter && !config.event_id_filter->empty())
            _logger.info("   number of selected eventID=%d", config.event_id_filter->count());
        else
            _logger.info("   all eventID selected");
        _logger.info("   filter=%s", config.filter.c_str());
        _logger.info("   formatter=%s", config.formatter.c_str());

        EventProcessContext process_context {
            .filter = _filters[config.filter],
            .formatter = _formatters[config.formatter]
        };

        if (!config.filter.empty() && process_context.filter == nullptr) {
            _logger.error("   *** filter '%s' not configured ***", config.filter.c_str());
            return;
        }

        if (config.formatter.empty()) {
            _logger.error("   *** formatter not defined ***", config.filter.c_str());
            return;
        }
        else if (process_context.formatter == nullptr) {
            _logger.error("   *** formatter '%s' not configured ***", config.formatter.c_str());
            return;
        }

        try {
            _subscriptions.push_back(std::make_unique<EventSubscription>(
                config.id,
                config.channel,
                config.event_id_filter->query(),
                process_context)
            );
        }
        catch (const utl::os_error& e) {
            if (e.error_code() == ERROR_ACCESS_DENIED)
                _logger.error("   *** access denied ***");
            else if (e.error_code() == ERROR_EVT_INVALID_QUERY)
                _logger.error("   *** invalid query, ID list too complex ***");
            else
                throw;
        }
    }


	unsigned int EventCollector::run()
	{
        _logger.info("Starting event collector...");
		auto event_handler = [&](const EventProcessContext& ctx, const EventLogHandle& event) {
			return handle_event(ctx, event);
		};

        try {
            // Main loop: drain all subscriptions, then wait for a signal or queue stop.
            while (!interrupted()) {
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
                wait_event(5000);
            }

            _logger.info("Event collector stopped");
        }
        catch (const utl::os_error& e) {
            _logger.error("Aborting event collector.");
            _logger.error(e.what());
        }
        catch (...) {
            _logger.error("Aborting event collector, unexpected error");
        }

		return 0;
	}


    void EventCollector::stop()
    {
        _logger.info("Stopping event collector...");
        for (auto& subscription : _subscriptions)
            subscription->cancel();

        EventThread::stop();
    }


	ProcessResult EventCollector::handle_event(const EventProcessContext& ctx, const EventLogHandle& event)
	{
		auto emb = std::make_unique<EventMessageBuilder>(ctx.max_message_size());

		// Process the event and push it into the message queue.
        ProcessResult result = _processor.process(ctx, event, *emb);
        if (result == ProcessResult::Success) {
            if (!push_message(std::move(emb)))
                result = ProcessResult::Failed;
        }

        return result;
	}


	bool EventCollector::wait_event(const DWORD milli_secs)
	{
        //TODO: create a global const
		std::array<::HANDLE, 16> handles{};
		::DWORD handle_count = 0;

		handles[handle_count++] = stop_signal().handle();

		for (auto& subscription : _subscriptions) {
			if (!subscription->is_cancelled())
				handles[handle_count++] = subscription->get_signal().handle();
		}

        switch (::WaitForMultipleObjects(handle_count, handles.data(), FALSE, milli_secs)) {
        case WAIT_FAILED:
            throw utl::os_error("EventCollector: WaitForMultipleObjects error", ::GetLastError());

        case WAIT_TIMEOUT:
            return false;

        default:
            return true;
        }
	}

}