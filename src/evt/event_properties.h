/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#pragma once

#include <bitset>
#include <string>


namespace wlf::evt {

    enum class EventSystemProperty {
        ProviderName = 0,
        ProviderGuid = 1,
        EventID = 2,
        Qualifiers = 3,
        Level = 4,
        Task = 5,
        Opcode = 6,
        Keywords = 7,
        TimeCreated = 8,
        EventRecordID = 9,
        ActivityID = 10,
        RelatedActivityID = 11,
        ProcessID = 12,
        ThreadID = 13,
        Channel = 14,
        Computer = 15,
        AccountName = 16,
        Domain = 17,
        AccountType = 18,
        Version = 19
    };

    constexpr size_t EventSystemPropertyCount = 20;

    const char8_t* to_string(EventSystemProperty property) noexcept;
    bool from_string(const std::string& str, EventSystemProperty& property) noexcept;


    class EventSystemProperties
    {
    public:
        EventSystemProperties() = default;
        
        static EventSystemProperties all() noexcept;
        bool contains(EventSystemProperty property) const noexcept;
        void add(EventSystemProperty property) noexcept;
        void exclude(EventSystemProperty property) noexcept;

    private:
        std::bitset<EventSystemPropertyCount> _properties{};
    };

}