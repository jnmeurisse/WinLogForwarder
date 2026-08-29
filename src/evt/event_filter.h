/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
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