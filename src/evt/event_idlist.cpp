/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_idlist.h"
#include <sstream>


namespace wlf::evt {
    
    EventIDList::EventIDList(const std::wstring provider_name)
        : _provider_name(provider_name)
        , _list()
    {
    }


    void EventIDList::add(::DWORD lower_id, ::DWORD upper_id)
    {
        if (lower_id <= upper_id)
            _list.emplace_back(EventIDPair{ lower_id, upper_id });
    }


    void EventIDList::add(::DWORD id)
    {
        add(id, id);
    }


    size_t EventIDList::count() const noexcept
    {
        size_t count = 0;

        for (const auto& id_pair : _list) {
            count += (static_cast<size_t>(id_pair.upper_id) - id_pair.upper_id + 1);
        }

        return count;
    }

    
    std::wstring EventIDList::query() const
    {
        if (empty())
            return L"*";

        std::wostringstream xpath;

        xpath << L"*[System[";

        // Add an optional provider constraint
        if (!_provider_name.empty())
            xpath << L"Provider[@Name='" << _provider_name << L"'] and '";

        const bool need_outer_parenthesis = _list.size() > 1;
        if (need_outer_parenthesis)
            xpath << L"(";

        bool next_id = false;
        for (const auto& id_pair : _list) {
            if (next_id)
                xpath << L" or ";

            if (id_pair.is_singleton())
                xpath << L"EventID=" << id_pair.lower_id;
            else
                xpath << L"(EventID>=" << id_pair.lower_id
                << L" and EventID<=" << id_pair.upper_id
                << L")";

            next_id = true;
        }

        if (need_outer_parenthesis)
            xpath << L")";

        xpath << L"]]";

        return xpath.str();
    }


}