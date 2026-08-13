#include "event_filter.h"

namespace wlf::evt {

	bool EventFilter::filter(const EventData& event_data, EventMessageBuilder& emb) const noexcept
	{
		return false;
	}

}