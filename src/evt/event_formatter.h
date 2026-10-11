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

	class EventFormatter abstract {
	public:
        explicit EventFormatter(size_t max_msg_size);
		virtual ~EventFormatter() = default;

        inline size_t max_message_size() const noexcept { return _max_msg_size; }
        virtual bool format(const EventData& evd, EventMessageBuilder& emb) const noexcept = 0;

    private:
        const size_t _max_msg_size;
    };

	using EventFormatterPtr = std::shared_ptr<EventFormatter>;
}
