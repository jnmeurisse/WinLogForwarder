/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_properties.h"

#include <string>
#include <map>
#include <utl/string.h>


namespace wlf::evt {


    const char8_t* to_string(EventSystemProperty property) noexcept
    {
        static const std::map<EventSystemProperty, const char8_t*> properties = {
            {EventSystemProperty::AccountName, u8"AccountName"},
            {EventSystemProperty::ProviderName, u8"ProviderName"},
            {EventSystemProperty::ProviderGuid, u8"ProviderGuid" },
            {EventSystemProperty::EventID, u8"EventID"},
            {EventSystemProperty::Qualifiers, u8"Qualifiers"},
            {EventSystemProperty::Level, u8"Level"},
            {EventSystemProperty::Task, u8"Task"},
            {EventSystemProperty::Opcode, u8"Opcode"},
            {EventSystemProperty::Keywords, u8"Keywords"},
            {EventSystemProperty::TimeCreated, u8"TimeCreated"},
            {EventSystemProperty::EventRecordID, u8"EventRecordID"},
            {EventSystemProperty::ActivityID, u8"ActivityID"},
            {EventSystemProperty::RelatedActivityID, u8"RelatedActivityID"},
            {EventSystemProperty::ProcessID, u8"ProcessID"},
            {EventSystemProperty::ThreadID, u8"ThreadID"},
            {EventSystemProperty::Channel, u8"Channel"},
            {EventSystemProperty::Computer, u8"Computer"},
            {EventSystemProperty::AccountName, u8"AccountName"},
            {EventSystemProperty::Domain, u8"Domain"},
            {EventSystemProperty::AccountType, u8"AccountType"},
            {EventSystemProperty::Version, u8"Version"},
        };

        return properties.at(property);
    }


    bool from_string(const std::string& str, EventSystemProperty& property) noexcept
    {
        static const std::map<std::string, EventSystemProperty, utl::icomp> properties = {
            { "AccountName", EventSystemProperty::AccountName },
            { "ProviderName", EventSystemProperty::ProviderName },
            { "ProviderGuid",EventSystemProperty::ProviderGuid },
            { "EventID", EventSystemProperty::EventID },
            { "Qualifiers", EventSystemProperty::Qualifiers },
            { "Level", EventSystemProperty::Level },
            { "Task", EventSystemProperty::Task },
            { "Opcode", EventSystemProperty::Opcode },
            { "Keywords", EventSystemProperty::Keywords },
            { "TimeCreated", EventSystemProperty::TimeCreated },
            { "EventRecordID", EventSystemProperty::EventRecordID },
            { "ActivityID", EventSystemProperty::ActivityID },
            { "RelatedActivityID", EventSystemProperty::RelatedActivityID },
            { "ProcessID", EventSystemProperty::ProcessID },
            { "ThreadID", EventSystemProperty::ThreadID },
            { "Channel", EventSystemProperty::Channel},
            { "Computer", EventSystemProperty::Computer },
            { "AccountName", EventSystemProperty::AccountName },
            { "Domain", EventSystemProperty::Domain },
            { "AccountType", EventSystemProperty::AccountType },
            { "Version", EventSystemProperty::Version }
        };

        auto p = properties.find(str.c_str());
        if (p != properties.end())
            property = p->second;

        return p != properties.end();
    }


    EventSystemProperties EventSystemProperties::all() noexcept
    {
        EventSystemProperties result;

        result._properties.set();
        return result;
    }


    bool EventSystemProperties::contains(EventSystemProperty property) const noexcept
    {
        return _properties.test(static_cast<std::size_t>(property));
    }


    void EventSystemProperties::add(EventSystemProperty property) noexcept
    {
        _properties.set(static_cast<std::size_t>(property));
    }


    void EventSystemProperties::exclude(EventSystemProperty property) noexcept
    {
        _properties.reset(static_cast<std::size_t>(property));
    }

}