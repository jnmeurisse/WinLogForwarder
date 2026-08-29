/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_facility.h"
#include <algorithm>
#include <map>

#include "utl/string.h"


namespace wlf::evt {

    const char8_t* to_string(EventFacility facility) noexcept
    {
        static const std::map<EventFacility, const char8_t*> facilities = {
            { EventFacility::local0, u8"local0"},
            { EventFacility::local1, u8"local1"},
            { EventFacility::local2, u8"local2"},
            { EventFacility::local3, u8"local3"},
            { EventFacility::local4, u8"local4"},
            { EventFacility::local5, u8"local5"},
            { EventFacility::local6, u8"local6"},
            { EventFacility::local7, u8"local7"}
        };

        return facilities.at(facility);
    }


    unsigned int to_priority(EventFacility facility, unsigned int severity) noexcept
    {
        const unsigned int facility_value = static_cast<unsigned int>(facility) + 16;
        return facility_value * 8 + std::min(7u, severity);
    }


    bool from_string(const std::string& str, EventFacility& facility) noexcept
    {
        static const std::map<std::string, EventFacility, utl::icomp> facilities = {
            {"local0", EventFacility::local0},
            {"local1", EventFacility::local1},
            {"local2", EventFacility::local2},
            {"local3", EventFacility::local3},
            {"local4", EventFacility::local4},
            {"local5", EventFacility::local5},
            {"local6", EventFacility::local6},
            {"local7", EventFacility::local7},
        };

        auto p = facilities.find(str);
        if (p != facilities.end())
            facility = p->second;

        return p != facilities.end();
    }

}