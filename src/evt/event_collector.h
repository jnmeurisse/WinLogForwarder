#pragma once

#include <chrono>

#include "evt/event_processor.h"
#include "evt/event_queue.h"
#include "evt/event_subscription.h"
#include "evt/event_thread.h"


namespace wlf::evt {
	using namespace std::chrono;

    using EventCollectorConfig = EventProcessor::Config;
    using EventSubscriptionConfig = EventSubscription::Config;

	class EventCollector : public EventThread {
	public:
		EventCollector(EventQueue& queue, const EventCollectorConfig& config);
		void add_subscription(const EventSubscriptionConfig& config);

        void stop() noexcept override;

	protected:
		unsigned int run() override;

	private:
		EventProcessor _processor;
		std::vector<EventSubscriptionPtr> _subscriptions;
		steady_clock::time_point _watchdog;

		ProcessResult handle_event(const EventProcessor::Context& ctx, const EventLogHandle& event);
		DWORD wait_event(DWORD timeout);
	};

}