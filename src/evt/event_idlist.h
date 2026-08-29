#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

#include "evt/event_types.h"


namespace wlf::evt{

    struct EventIDPair {
        ::DWORD lower_id;
        ::DWORD upper_id;

        bool is_singleton() const { return lower_id == upper_id; }
    };


    class EventIDList : public EventIDFilter
    {
    public:
        explicit EventIDList(const std::wstring provider_name);

        void add(::DWORD lower_id, ::DWORD upper_id);
        void add(::DWORD id);

        /**
         * Returns the total number of event IDs selected in this list.
         */
        size_t count() const noexcept override;

        bool empty() const noexcept override { return _list.empty(); }

        /**
         * Builds a query string for this event ID list.
         */
        std::wstring query() const override;

    private:
        std::wstring _provider_name;
        std::vector<EventIDPair> _list;
    };

}