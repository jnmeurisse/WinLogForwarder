/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_filter.h"

namespace wlf::evt {

	bool EventFilter::filter(const EventData& event_data, EventMessageBuilder& emb) const noexcept
	{
		return false;
	}

}