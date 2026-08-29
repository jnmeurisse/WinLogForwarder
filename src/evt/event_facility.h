#pragma once

#include <string>


namespace wlf::evt {

    enum class EventFacility {
        local0 = 0,
        local1,
        local2,
        local3,
        local4,
        local5,
        local6,
        local7
    };

    constexpr size_t EventFacilityCount = 8;

    const char8_t* to_string(EventFacility facility) noexcept;
    unsigned int to_priority(EventFacility facility, unsigned int severity) noexcept;
    bool from_string(const std::string& str, EventFacility& facility) noexcept;
    
}