#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
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
        const wchar_t* provider_name;       // EvtSystemProviderName        :EvtVarTypeString
        const ::GUID* provider_guid;        // EvtSystemProviderGuid        :EvtVarTypeGuid  
        uint16_t event_id;                  // EvtSystemEventID             :EvtVarTypeUInt16
        uint16_t qualifiers;                // EvtSystemQualifiers          :EvtVarTypeUInt16
        uint8_t level;                      // EvtSystemLevel               :EvtVarTypeUInt8
        uint16_t task;                      // EvtSystemTask                :EvtVarTypeUInt16
        uint8_t opcode;                     // EvtSystemOpcode              :EvtVarTypeUInt8
        uint64_t keywords;                  // EvtSystemKeywords            :EvtVarTypeHexInt64
        ::FILETIME time_created;            // EvtSystemTimeCreated         :EvtVarTypeFileTime
        uint64_t event_record_id;           // EvtSystemEventRecordId       :EvtVarTypeUInt64
        const GUID* activity_id;            // EvtSystemActivityID          :EvtVarTypeGuid
        const GUID* related_activity_id;    // EvtSystemRelatedActivityID   :EvtVarTypeGuid
        uint32_t process_id;                // EvtSystemProcessID           :EvtVarTypeUInt32
        uint32_t thread_id;                 // EvtSystemThreadID            :EvtVarTypeUInt32
        const wchar_t* channel;             // EvtSystemChannel             :EvtVarTypeString
        const wchar_t* computer;            // EvtSystemComputer            :EvtVarTypeString
        const ::SID* user_id;               // EvtSystemUserID              :EvtVarTypeSid
        uint8_t version;                    // EvtSystemVersion             :EvtVarTypeUInt8

        void clear() {
            provider_name = nullptr;
            provider_guid = nullptr;
            event_id = 0;
            qualifiers = 0;
            level = 0;
            task = 0;
            opcode = 0;
            keywords = 0;
            time_created = {};
            event_record_id = 0;
            activity_id = nullptr;
            related_activity_id = nullptr;
            process_id = 0;
            thread_id = 0;
            channel = nullptr;
            computer = nullptr;
            user_id = nullptr;
            version = 0;
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