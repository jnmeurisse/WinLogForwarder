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

#include <cstdint>
#include <optional>

#include "utl/pugixml.hpp"

namespace wlf::evt
{
    /**
     * User account information associated with a Windows event.
     * (resolved from system_data.user_id)
     *
     * The strings are non-owning pointers. Their lifetime is managed by the
     * user cache.
     */
    struct AccountData {
        const wchar_t* name = nullptr;
        const wchar_t* domain = nullptr;
        const wchar_t* type = nullptr;
    };


    /**
     * System-level information extracted from a Windows Event Log record.
     *
     * Most members correspond directly to values exposed by the Windows
     * Event Log EvtSystem* identifiers.
     *
     * Pointer members are non-owning and must remain valid for the lifetime
     * of the data source from which they were extracted.
     */
    struct SystemData {
        const wchar_t* provider_name = nullptr;     // EvtSystemProviderName        :EvtVarTypeString
        const ::GUID* provider_guid = nullptr;      // EvtSystemProviderGuid        :EvtVarTypeGuid  
        std::optional<uint16_t> event_id;           // EvtSystemEventID             :EvtVarTypeUInt16
        std::optional<uint16_t> qualifiers;         // EvtSystemQualifiers          :EvtVarTypeUInt16
        std::optional<uint8_t> level;               // EvtSystemLevel               :EvtVarTypeUInt8
        std::optional<uint16_t> task;               // EvtSystemTask                :EvtVarTypeUInt16
        std::optional<uint8_t> opcode;              // EvtSystemOpcode              :EvtVarTypeUInt8
        std::optional<uint64_t> keywords;           // EvtSystemKeywords            :EvtVarTypeHexInt64
        ::FILETIME time_created{};                  // EvtSystemTimeCreated         :EvtVarTypeFileTime
        std::optional<uint64_t> event_record_id;    // EvtSystemEventRecordId       :EvtVarTypeUInt64
        const GUID* activity_id = nullptr;          // EvtSystemActivityID          :EvtVarTypeGuid
        const GUID* related_activity_id = nullptr;  // EvtSystemRelatedActivityID   :EvtVarTypeGuid
        std::optional<uint32_t> process_id;         // EvtSystemProcessID           :EvtVarTypeUInt32
        std::optional<uint32_t> thread_id;          // EvtSystemThreadID            :EvtVarTypeUInt32
        const wchar_t* channel = nullptr;           // EvtSystemChannel             :EvtVarTypeString
        const wchar_t* computer = nullptr;          // EvtSystemComputer            :EvtVarTypeString
        ::PSID user_id = nullptr;                   // EvtSystemUserID              :EvtVarTypeSid
        std::optional<uint8_t> version;             // EvtSystemVersion             :EvtVarTypeUInt8
    };

    /**
     * Data extracted from a Windows Event Log record.
     *
     * The structure contains the event's system metadata, optional user
     * account information, and its XML representation.
     *
     * The XML document owns its XML nodes. Pointer members in system_data
     * and account_data are non-owning and are not managed by this structure.
     */
    struct EventData {
        /* System Data */
        SystemData system_data;

        /* User account information, initialized when user_id is non null */
        AccountData account_data;

        /* XML representation of the event. */
		pugi::xml_document xml_doc;
    };

}