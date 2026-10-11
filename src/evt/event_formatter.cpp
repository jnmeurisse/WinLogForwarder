/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_formatter.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "evt/event_properties.h"
#include "evt/event_facility.h"
#include "utl/pugixml.hpp"


namespace wlf::evt {

    EventFormatter::EventFormatter(size_t max_msg_size)
        : _max_msg_size(max_msg_size)
    {
    }

}