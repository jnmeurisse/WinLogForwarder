/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "evt/event_filter.h"
#include "evt/event_formatter.h"
#include "evt/event_processor.h"
#include "evt/event_queue.h"
#include "evt/event_subscription.h"
#include "evt/event_types.h"
#include "evt/event_thread.h"


namespace wlf::evt {

	class EventCollector : public EventThread {
	public:
		EventCollector(EventQueue& queue, const EventCollectorConfig& config);

        void add_filter(const EventFilterConfig& config);
        void add_formatter(const EventFormatterConfig& config);
		void add_subscription(const EventSubscriptionConfig& config);

        void stop() override;

	protected:
		unsigned int run() override;

	private:
		EventProcessor _processor;
		std::vector<EventSubscriptionPtr> _subscriptions;
        std::unordered_map<std::string, EventFilterPtr> _filters;
        std::unordered_map<std::string, EventFormatterPtr> _formatters;

		ProcessResult handle_event(const EventProcessContext& ctx, const EventLogHandle& event);

        /**
         * Waits that an event is available for one of the subscription.
         * 
         * @param milli_secs 
         * @return true if an event is available, false in case of timeout
         * 
         * @throw os_error
         */
        bool wait_event(DWORD milli_secs);
	};

}