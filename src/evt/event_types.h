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

#include <memory>
#include <string>
#include "evt/event_properties.h"
#include "evt/event_facility.h"
#include "net/endpoint.h"
#include "utl/user_cache.h"


namespace wlf::evt {

    enum class EventForwarderProtocol {
        UDP,
        TCP,
        TLS
    };


    struct EventForwarderConfig {
        // Message collector host name or IP address and port
        net::Endpoint collector;

        // Communication protocol
        evt::EventForwarderProtocol protocol;

        // Certificates when TLS protocol is configured
        std::string ca_certificate;
        std::string client_certificate;
    };


    struct EventProcessorConfig {
        ::DWORD system_data_buffer_size_limit;
        ::DWORD event_data_buffer_size_limit;
        utl::UserCache& user_cache;
    };

    struct EventCollectorConfig {
        EventProcessorConfig processor_config;
    };

    struct EventFilterConfig {

    };

    struct EventFormatterConfig {
        // The formatter Id (used only for debugging)
        std::string id;

        // Syslog facility used when computing PRI
        EventFacility facility;

        // Hostname 
        std::wstring hostname;

        // Application name
        std::wstring app_name;

        // A set of standard Windows Event System properties to include
        // to the Structured Data Element output.
        EventSystemProperties system_properties;

        std::wstring sd_id_sys;
        std::wstring sd_id_evd;

        size_t max_syslog_sd_length;
        size_t max_syslog_msg_length;
    };

    class EventIDFilter abstract {
    public:
        virtual size_t count() const noexcept = 0;
        virtual bool empty() const noexcept = 0;
        virtual std::wstring query() const = 0;
    };

    struct EventSubscriptionConfig {
        // The subscription Id (used only for debugging)
        std::string id;

        // The event channel to subscribe to (e.g., L"Application").
        std::wstring channel;

        // An optional provider name
        std::wstring provider_name;

        // A list of selected event IDs (empty if all IDs are selected)
        std::shared_ptr<EventIDFilter> event_id_filter;

        // An optional filter procedure (no filter if empty)
        std::string filter;

        // A format procedure
        std::string formatter;
    };

}