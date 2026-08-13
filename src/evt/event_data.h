#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <optional>

#include "utl/pugixml.hpp"

namespace wlf::evt
{
    struct account_data_t {
        const wchar_t* name;
        const wchar_t* domain;
        const wchar_t* type;

        void clear() {
            name = nullptr;
            domain = nullptr;
            type = nullptr;
        }
    };


    struct system_data_t {
        const wchar_t* provider_name;           // EvtSystemProviderName        :EvtVarTypeString
        const ::GUID* provider_guid;            // EvtSystemProviderGuid        :EvtVarTypeGuid  
        std::optional<uint16_t> event_id;       // EvtSystemEventID             :EvtVarTypeUInt16
        std::optional<uint16_t> qualifiers;     // EvtSystemQualifiers          :EvtVarTypeUInt16
        std::optional<uint8_t> level;           // EvtSystemLevel               :EvtVarTypeUInt8
        std::optional<uint16_t> task;           // EvtSystemTask                :EvtVarTypeUInt16
        std::optional<uint8_t> opcode;          // EvtSystemOpcode              :EvtVarTypeUInt8
        std::optional<uint64_t> keywords;       // EvtSystemKeywords            :EvtVarTypeHexInt64
        ::FILETIME time_created;                // EvtSystemTimeCreated         :EvtVarTypeFileTime
        std::optional<uint64_t> event_record_id;// EvtSystemEventRecordId       :EvtVarTypeUInt64
        const GUID* activity_id;                // EvtSystemActivityID          :EvtVarTypeGuid
        const GUID* related_activity_id;        // EvtSystemRelatedActivityID   :EvtVarTypeGuid
        std::optional<uint32_t> process_id;     // EvtSystemProcessID           :EvtVarTypeUInt32
        std::optional<uint32_t> thread_id;      // EvtSystemThreadID            :EvtVarTypeUInt32
        const wchar_t* channel;                 // EvtSystemChannel             :EvtVarTypeString
        const wchar_t* computer;                // EvtSystemComputer            :EvtVarTypeString
        ::PSID user_id;                         // EvtSystemUserID              :EvtVarTypeSid
        std::optional<uint8_t> version;         // EvtSystemVersion             :EvtVarTypeUInt8

        void clear() {
            provider_name = nullptr;
            provider_guid = nullptr;
            event_id.reset();
            qualifiers.reset();
            level.reset();
            task.reset();
            opcode.reset();
            keywords.reset();
            time_created = {0, 0};
            event_record_id.reset();
            activity_id = nullptr;
            related_activity_id = nullptr;
            process_id.reset();
            thread_id.reset();
            channel = nullptr;
            computer = nullptr;
            user_id = nullptr;
            version.reset();
        }
    };


    // Definition of data extracted from a Windows Event.
    struct EventData {
        /* System Data */
        system_data_t system_data;

        /* User account information, initialized when user_id is non null */
        account_data_t account_data;

        /* */
		pugi::xml_document xml_doc;         // not a const, formatter could change it

        void clear() {
            system_data.clear();
            account_data.clear();
            xml_doc.reset();
        }
    };

}