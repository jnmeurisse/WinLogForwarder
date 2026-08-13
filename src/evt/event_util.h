#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <unordered_set>
#include "utl/options.h"

namespace wlf::evt {

    enum class EventSystemProperty {
        ProviderName = 0,
        ProviderGuid,
        EventID,
        Qualifiers,
        Level,
        Task,
        Opcode,
        Keywords,
        TimeCreated,
        EventRecordID,
        ActivityID,
        RelatedActivityID,
        ProcessID,
        ThreadID,
        Channel,
        Computer,
        AccountName,
        Domain,
        AccountType,
        Version
    };

    const char8_t* EventSystemPropertyName(EventSystemProperty property) noexcept;


    class EventSystemProperties : public utl::Options<EventSystemProperty>
    {
    };


    using EventIdFilter = std::unordered_set<::DWORD>;


}