#pragma once

#include <memory>
#include "evt/event_data.h"
#include "evt/event_message.h"


namespace wlf::evt {

	class EventFilter {
	public:
		virtual ~EventFilter() = default;
		virtual bool filter(const EventData& event_data, EventMessageBuilder& emb) const noexcept;
	};

	using EventFilterPtr = std::shared_ptr<EventFilter>;

}	