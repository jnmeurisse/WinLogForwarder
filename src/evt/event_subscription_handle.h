/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>

#include <span>
#include <string>
#include "evt/event_handle.h"
#include "utl/signal.h"


namespace wlf::evt {

    class EventSubscriptionHandle : public EventHandle
    {
    public:
        static EventSubscriptionHandle create(const utl::Signal& signal, const std::wstring& channel, const std::wstring& query);

        bool cancel() const noexcept;
        bool next(std::span<::EVT_HANDLE> event_handles, ::DWORD timeout, ::DWORD& count) const noexcept;

    private:
        using EventHandle::EventHandle;
    };

}