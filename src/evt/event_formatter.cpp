/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_formatter.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "evt/event_properties.h"
#include "evt/event_facility.h"
#include "utl/pugixml.hpp"


namespace wlf::evt {

    // Replacement character for non PRINTUSASCII character
    constexpr wchar_t RFC5424_REPLACEMENT_CHAR = L'_';

    // RFC5424 NIL value
    constexpr char8_t NILVALUE = u8'-';

    // RFC5424 SP value
    constexpr char8_t SP = u8' ';

    // Maximum length of PRINTUSASCII fields
    constexpr size_t HOSTNAME_LENGTH = 255;
    constexpr size_t APP_NAME_LENGTH = 48;
    constexpr size_t PROCID_LENGTH = 128;
    constexpr size_t MSGID_LENGTH = 32;
    constexpr size_t SD_NAME_LENGTH = 32;
    constexpr size_t MAX_ASCII_FIELD_LENGTH = 512;

    // SD-NAME = 1*32PRINTUSASCII
    //              ; except '=', SP, ']', %d34(")
    constexpr std::wstring_view SD_NAME_EXCLUDE_CHARS = L"= ]\"";


    // Forward declaration
    static bool append_printusascii(EventMessageBuilder& emb, std::wstring_view str, size_t count, std::wstring_view excluded, wchar_t replacement) noexcept;
    static bool append_printusascii_value(EventMessageBuilder& emb, const std::wstring& str, size_t count) noexcept;
    static bool append_sd_id(EventMessageBuilder& emb, const std::wstring& sd_id);

    /**
     * A RFC5424 Structured Data Parameter Name containing only USASCII characters.
     * This class can be used to store parameter names that ARE valid.
    */
    class SDAsciiParamName final : public SDParamName
    {
    public:
        SDAsciiParamName(const char8_t* name) noexcept : _name(name) {}

        bool append(EventMessageBuilder& emb) const noexcept override
        {
            return emb.append(_name);
        }

        explicit operator bool() const noexcept override
        {
            return _name.size() > 0 && _name[0] != u8'\0';
        }

    private:
        std::u8string_view _name;
    };


    /**
     * A RFC5424 Structured Data Parameter Name containing any characters.
     * This class MUST be used to store parameter names that have not be validated.
    */
    class SDWcharParamName final : public SDParamName
    {
    public:
        SDWcharParamName(const wchar_t* name) noexcept : _name(name) {}

        bool append(EventMessageBuilder& emb) const noexcept override
        {
            // Invalid characters are replaced by '_'
            //
            return append_printusascii(
                emb, 
                _name, SD_NAME_LENGTH,
                SD_NAME_EXCLUDE_CHARS, RFC5424_REPLACEMENT_CHAR
            );
        }

        explicit operator bool() const noexcept override
        {
            return _name.size() > 0 && _name[0] != L'\0';
        }

    private:
        std::wstring_view _name;
    };


    // System Property Names 
    static const SDAsciiParamName PROVIDER_NAME(to_string(EventSystemProperty::ProviderName));
    static const SDAsciiParamName PROVIDER_GUID(to_string(EventSystemProperty::ProviderGuid));
    static const SDAsciiParamName EVENT_ID(to_string(EventSystemProperty::EventID));
    static const SDAsciiParamName QUALIFIERS(to_string(EventSystemProperty::Qualifiers));
    static const SDAsciiParamName LEVEL(to_string(EventSystemProperty::Level));
    static const SDAsciiParamName TASK(to_string(EventSystemProperty::Task));
    static const SDAsciiParamName OPCODE(to_string(EventSystemProperty::Opcode));
    static const SDAsciiParamName KEYWORDS(to_string(EventSystemProperty::Keywords));
    static const SDAsciiParamName TIME_CREATED(to_string(EventSystemProperty::TimeCreated));
    static const SDAsciiParamName EVENT_RECORD_ID(to_string(EventSystemProperty::EventRecordID));
    static const SDAsciiParamName ACTIVITY_ID(to_string(EventSystemProperty::ActivityID));
    static const SDAsciiParamName RELATED_ACTIVITY_ID(to_string(EventSystemProperty::RelatedActivityID));
    static const SDAsciiParamName PROCESS_ID(to_string(EventSystemProperty::ProcessID));
    static const SDAsciiParamName THREAD_ID(to_string(EventSystemProperty::ThreadID));
    static const SDAsciiParamName CHANNEL(to_string(EventSystemProperty::Channel));
    static const SDAsciiParamName COMPUTER(to_string(EventSystemProperty::Computer));
    static const SDAsciiParamName ACCOUNT_NAME(to_string(EventSystemProperty::AccountName));
    static const SDAsciiParamName DOMAIN_NAME(to_string(EventSystemProperty::Domain));
    static const SDAsciiParamName ACCOUNT_TYPE(to_string(EventSystemProperty::AccountType));
    static const SDAsciiParamName VERSION(to_string(EventSystemProperty::Version));


    /**
     * Helper function to map a Windows Event Log level to a Syslog severity.
     * Windows: 1=Critical, 2=Error, 3=Warning, 4=Info, 5=Debug
     * Syslog: 0=Emergency ... 7=Debug
     */
    static inline unsigned int level_to_syslog_severity(std::optional<::UINT8> level) 
    {
        unsigned int severity = 6;
        if (level.has_value()) {
            switch (level.value()) {
            case 1: 
                severity = 2; // Critical
                break;
            case 2: 
                severity = 3; // Error
                break;
            case 3:
                severity = 4; // Warning
                break;
            case 4: 
                severity = 6; // Informational
                break;

            case 5:
                severity = 7; // Debug
                break;

            default:
                severity = 6; // Default to Info
                break;
            }
        }
        return severity;
    }


    static inline bool is_printusascii(wchar_t wch)
    {
        //
        // PRINTUSASCII = %d33-126
        //
        return (wch >= 33) && (wch <= 126);
    }


    RFC5424EventFormatter::RFC5424EventFormatter(const EventFormatterConfig& config) noexcept
        : EventFormatter()
        , _config(config)
    {
    }


    size_t RFC5424EventFormatter::max_message_size() const noexcept
    {
        return _config.max_syslog_msg_length;
    }


    bool RFC5424EventFormatter::format(
        const EventData& event_data,
        EventMessageBuilder& emb
    ) const noexcept
    {
        // 1. HEADER
        if (!append_header(emb, event_data))
            return false;

        // 2. STRUCTURED-DATA
        if (!append_system_sd(emb, event_data))
            return false;

        //TODO: append meta (IP) etc... 

        // Optional: omit if it doesn't fit.
        append_event_sd(emb, event_data);

        // 3. Try to append UserData if present and enough space (RFC 5424 Payload)
        append_user_data(emb, event_data);

        return true;
    }


    bool RFC5424EventFormatter::append_header(
        EventMessageBuilder& emb,
        const evt::EventData& event_data
    ) const noexcept
    {
        const unsigned int severity = level_to_syslog_severity(event_data.system_data.level);

        auto savepoint = emb.savepoint();

        const bool append_status = 
            // 1. <PRI> and VERSION
            append_priority(emb, to_priority(_config.facility, severity)) &&
            append_version(emb, 1) &&
            append_space(emb) &&

            // 2. TIMESTAMP of the formatted event data
            append_timestamp(emb) &&
            append_space(emb) &&

            // 3. HOSTNAME
            append_printusascii_value(emb, _config.hostname, HOSTNAME_LENGTH) &&
            append_space(emb) &&

            // 4. APP-NAME
            append_printusascii_value(emb, _config.app_name, APP_NAME_LENGTH) &&
            append_space(emb) &&

            // 5. PROCID
            append_id(emb, ::GetCurrentProcessId(), PROCID_LENGTH) &&
            append_space(emb) &&

            // 6. MSGID :: TODO
            append_id(emb, 0, MSGID_LENGTH);

        if (append_status)
            return savepoint.commit();

        return false;
    }


    inline bool RFC5424EventFormatter::append_space(
        EventMessageBuilder& emb
    ) const noexcept
    {
        //
        // SP = %d32
        //
        return emb.append(SP);
    }


    bool RFC5424EventFormatter::append_priority(
        EventMessageBuilder& emb,
        unsigned int priority
    ) const noexcept
    {
        //
        // PRI = "<" PRIVAL ">"
        // PRIVAL = 1 * 3DIGIT; range 0 .. 191
        //
        assert(priority <= 191);

        return
            emb.append(u8'<')       &&
            emb.append(priority, 3) &&
            emb.append(u8'>');
     }


    bool RFC5424EventFormatter::append_version(
        EventMessageBuilder& emb,
        unsigned int version
    ) const noexcept
    {
        //
        // VERSION = NONZERO - DIGIT 0 * 2DIGIT
        //
        assert(version > 0 && version <= 99);

        return emb.append(version, 2);
    }


    bool RFC5424EventFormatter::append_timestamp(
        EventMessageBuilder& emb
    ) const noexcept
    {
        SYSTEMTIME now;
        ::GetSystemTime(&now);

        return emb.append(now);
    }


    bool RFC5424EventFormatter::append_id(
        EventMessageBuilder& emb,
        unsigned int id,
        size_t count
    ) const noexcept
    {
        return emb.append(id, count);
    }


    bool RFC5424EventFormatter::append_system_sd(
        EventMessageBuilder& emb,
        const EventData& event_data
    ) const noexcept
    {
        auto savepoint = emb.savepoint();

        if (!emb.append(u8'['))
            return false;

        // Append the SD-ID
        if (!append_sd_id(emb, _config.sd_id_sys))
            return false;

        const SystemData& system_data = event_data.system_data;
        const AccountData& account_data = event_data.account_data;
        const EventSystemProperties& selected_properties = _config.system_properties;

        // Try to append selected system properties.
        // Each property is appended using the SD-PARAM RFC5424 syntax.
        //  SD-PARAM = PARAM-NAME "=" %d34 PARAM-VALUE %d34
        // The parameter is not appended if there is not enough space in the
        // message builder.  A committed SD-PARAM contains always a valid 
        // RFC-5424 SD-PARAM and keep at least 1 byte in the builder to append
        // the "]" character.

        if (selected_properties.contains(EventSystemProperty::ProviderName) && system_data.provider_name &&
            !append_sd_param(emb, PROVIDER_NAME, system_data.provider_name, _config.max_syslog_sd_length))
            return false;

        if (selected_properties.contains(EventSystemProperty::ProviderGuid) && system_data.provider_guid &&
            !append_sd_param(emb, PROVIDER_GUID, system_data.provider_guid))
            return false;

        if (selected_properties.contains(EventSystemProperty::EventID) && system_data.event_id &&
            !append_sd_param(emb, EVENT_ID, system_data.event_id.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Qualifiers) && system_data.qualifiers &&
            !append_sd_param(emb, QUALIFIERS, system_data.qualifiers.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Version) && system_data.version &&
            !append_sd_param(emb, VERSION, system_data.version.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Level) && system_data.level && 
            !append_sd_param(emb, LEVEL, system_data.level.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Task) && system_data.task &&
            !append_sd_param(emb, TASK, system_data.task.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Opcode) && system_data.opcode &&
            !append_sd_param(emb, OPCODE, system_data.opcode.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Keywords) && system_data.keywords &&
            !append_sd_param(emb, KEYWORDS, system_data.keywords.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::TimeCreated) &&
            !append_sd_param(emb, TIME_CREATED, system_data.time_created))
            return false;

        if (selected_properties.contains(EventSystemProperty::EventRecordID) && system_data.event_record_id &&
            !append_sd_param(emb, EVENT_RECORD_ID, system_data.event_record_id.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::ActivityID) && system_data.activity_id &&
            !append_sd_param(emb, ACTIVITY_ID, system_data.activity_id))
            return false;

        if (selected_properties.contains(EventSystemProperty::RelatedActivityID) && system_data.related_activity_id &&
            !append_sd_param(emb, RELATED_ACTIVITY_ID, system_data.related_activity_id))
            return false;

        if (selected_properties.contains(EventSystemProperty::ProcessID) && system_data.process_id &&
            !append_sd_param(emb, PROCESS_ID, system_data.process_id.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::ThreadID) && system_data.thread_id &&
            !append_sd_param(emb, THREAD_ID, system_data.thread_id.value()))
            return false;

        if (selected_properties.contains(EventSystemProperty::Channel) && system_data.channel &&
            !append_sd_param(emb, CHANNEL, system_data.channel, _config.max_syslog_sd_length))
            return false;

        if (selected_properties.contains(EventSystemProperty::Computer) && system_data.computer &&
            !append_sd_param(emb, COMPUTER, system_data.computer, _config.max_syslog_sd_length))
            return false;

        if (system_data.user_id) {
            if (selected_properties.contains(EventSystemProperty::AccountName) && account_data.name &&
                !append_sd_param(emb, ACCOUNT_NAME, account_data.name, _config.max_syslog_sd_length))
                return false;

            if (selected_properties.contains(EventSystemProperty::Domain) && account_data.domain &&
                !append_sd_param(emb, DOMAIN_NAME, account_data.domain, _config.max_syslog_sd_length))
                return false;

            if (selected_properties.contains(EventSystemProperty::AccountType) && account_data.type &&
                !append_sd_param(emb, ACCOUNT_TYPE, account_data.type, _config.max_syslog_sd_length))
                return false;
        }

        if (!emb.append(u8']'))
            return false;

        return savepoint.commit();
    }


    void RFC5424EventFormatter::append_event_sd(
        EventMessageBuilder& emb,
        const EventData& event_data
    ) const noexcept
    {
        using namespace pugi;

        xml_node event_node = event_data.xml_doc.first_child();
        if (!event_node || std::wcscmp(event_node.name(), L"Event") != 0)
            return;

        // A lambda that checks if a node name = "EventData"
        auto is_event_data_node = [](const xml_node& node) {
            return std::wcscmp(node.name(), L"EventData") == 0;
        };

        xml_node event_data_node = event_node.find_child(is_event_data_node);
        if (!event_data_node)
            return;

        auto savepoint = emb.savepoint();

        if (!emb.append(u8'['))
            return;

        if (!append_sd_id(emb, _config.sd_id_evd))
            return;

        for (xml_node node : event_data_node) {
            if (std::wcscmp(node.name(), L"Data") !=0)
                continue;

            const wchar_t* param_name = L"data";
            xml_attribute attribute = node.attribute(L"Name");
            if (attribute)
                param_name = attribute.value();

            if (!append_sd_param(
                emb,
                SDWcharParamName(param_name),
                node.text().as_string(nullptr),
                _config.max_syslog_sd_length))
                break;
        }

        if (!emb.append(u8']'))
            return;

        savepoint.commit();
    }

    bool RFC5424EventFormatter::append_sd_param(
        EventMessageBuilder& emb,
        const SDParamName& name,
        const wchar_t* value,
        size_t max_chars
    ) const noexcept
    {
        if (!name || !value || *value == L'\0')
            return true;

        return
            emb.append(SP) &&
            name.append(emb) &&
            emb.append(u8"=\"") &&
            append_sd_value(emb, value, max_chars) &&
            emb.append(u8'"');
    }


    bool RFC5424EventFormatter::append_sd_param(
        EventMessageBuilder& emb,
        const SDParamName& name,
        const::FILETIME& ft
    ) const noexcept
    {
        if (!name)
            return true;

        ::SYSTEMTIME system_time;
        if (!::FileTimeToSystemTime(&ft, &system_time))
            return true;

        return
            emb.append(SP) &&
            name.append(emb) &&
            emb.append(u8"=\"") &&
            emb.append(system_time) &&
            emb.append(u8'"');
    }


    bool RFC5424EventFormatter::append_sd_param(
        EventMessageBuilder& emb,
        const SDParamName& name, 
        uint64_t value
    ) const noexcept
    {
        if (!name)
            return true;

        return
            emb.append(SP) &&
            name.append(emb) &&
            emb.append(u8"=\"") &&
            emb.append(value, std::numeric_limits<uint64_t>::digits10) &&
            emb.append(u8'"');
    }


    bool RFC5424EventFormatter::append_sd_param(
        EventMessageBuilder& emb,
        const SDParamName& name,
        const::GUID* value
    ) const noexcept
    {
        if (!name)
            return true;

        return
            emb.append(SP) &&
            name.append(emb) &&
            emb.append(u8"=\"") &&
            emb.append(value) &&
            emb.append(u8'"');
    }


    bool RFC5424EventFormatter::append_sd_value(
        EventMessageBuilder& emb,
        const wchar_t* const value,
        size_t max_code_units
    ) const noexcept
    {
        //
        // PARAM - VALUE = UTF - 8 - STRING; characters '"', '\' and
        //    ; ']' MUST be escaped.
        //

        // A lambda that checks whether escape is required 
        auto needs_escape = [](wchar_t c) {
            return (c == L'"' || c == L'\\' || c == L']');
        };

        // index in source string
        const wchar_t* p = value;

        // a flag that is true as long as text is appended to the message builder
        bool emb_append_ok = true;

        while (*p && max_code_units > 0 && emb_append_ok) {
            // Copy characters from the source string to a buffer until
            // the buffer is full.  Buffer is considered as full when the
            // filling index is greater than the buffer size.  The buffer
            // has twice the capacity so that we have always enough room
            // to store surrogate or escaped characters.
            constexpr size_t buffer_size = 1024;
            std::array<wchar_t, 2 * buffer_size> buffer;

            // index in the output buffer
            size_t j = 0;

            while (*p && max_code_units > 0 && j < buffer_size) {
                const wchar_t wch = *p++;

                if (IS_HIGH_SURROGATE(wch)) {
                    if (*p && IS_LOW_SURROGATE(*p)) {
                        buffer[j++] = wch;
                        buffer[j++] = *p++;
                    }
                    else {
                        buffer[j++] = RFC5424_REPLACEMENT_CHAR;
                    }
                }
                else if (IS_LOW_SURROGATE(wch)) {
                    buffer[j++] = RFC5424_REPLACEMENT_CHAR;
                }
                else {
                    if (needs_escape(wch))
                        buffer[j++] = L'\\';

                    buffer[j++] = wch;
                }

                --max_code_units;
            }

            if (j > 0)
                emb_append_ok = emb.write_chars(buffer.data(), j);
        }

        return emb_append_ok;
    }


    struct xml_string_writer : pugi::xml_writer
    {
        EventMessageBuilder& _emb;
        bool overflow = false;
 
        xml_string_writer(EventMessageBuilder& emb) noexcept : _emb(emb) {};

        void write(const void* data, size_t size) override
        {
            overflow |= !_emb.write_chars(static_cast<const wchar_t*>(data), size / sizeof(wchar_t));
        }
    };


    void RFC5424EventFormatter::append_user_data(
        EventMessageBuilder& emb,
        const EventData& event_data
    ) const noexcept
    {
        using namespace pugi;

        if (event_data.xml_doc.empty())
            return;

        xml_node event_node = event_data.xml_doc.first_child();
        if (!event_node || std::wcscmp(event_node.name(), L"Event") != 0)
            return;

        // A lambda that checks if a node name = "UserData"
        auto is_event_user_node = [](const xml_node& node) {
            return std::wcscmp(node.name(), L"UserData") == 0;
        };

        xml_node user_data_node = event_node.find_child(is_event_user_node);
        if (!user_data_node)
            return;

        auto savepoint = emb.savepoint();

        // Append delimiter and BOM
        if (!emb.append(u8" \xef\xbb\xbf"))
            return;

        // Append user data XML 
        xml_string_writer writer(emb);
        user_data_node.print(writer, L"", format_raw, pugi::encoding_wchar);
        if (writer.overflow)
            return;

        savepoint.commit();
        return;
    }

    static bool append_printusascii(
        EventMessageBuilder& emb,
        std::wstring_view str,
        size_t count,
        std::wstring_view excluded,
        wchar_t replacement
    ) noexcept
    {
        if (str.empty() || count == 0)
            return true;

        // Temporary buffer holding the transformed string
        std::array<wchar_t, MAX_ASCII_FIELD_LENGTH> buffer;
        assert(count <= buffer.size());
        
        // pointer in the input string
        const wchar_t* p = str.data();
        const wchar_t* const end = p + str.size();

        // index in the output buffer
        size_t i = 0;

        while (p < end && i < std::min(count, buffer.size())) {
            const wchar_t wch = *p++;

            if (IS_HIGH_SURROGATE(wch)) {
                if (p < end && IS_LOW_SURROGATE(*p))
                    ++p;
                buffer[i++] = replacement;
            }
            else if (IS_LOW_SURROGATE(wch))
                buffer[i++] = replacement;
            else if (!is_printusascii(wch))
                buffer[i++] = replacement;
            else if (excluded.find(wch) != std::wstring_view::npos)
                buffer[i++] = replacement;
            else
                buffer[i++] = wch;
        }

        return emb.write_chars(buffer.data(), i);
    }


    static bool append_printusascii_value(
        EventMessageBuilder& emb,
        const std::wstring& str,
        size_t count
    ) noexcept
    {
        if (str.empty())
            return emb.append(NILVALUE);
        return append_printusascii(
            emb,
            str, count,
            L"",
            RFC5424_REPLACEMENT_CHAR);
    }


    bool append_sd_id(EventMessageBuilder& emb, const std::wstring& sd_id)
    {
        return append_printusascii(
            emb,
            sd_id, SD_NAME_LENGTH,
            SD_NAME_EXCLUDE_CHARS,
            RFC5424_REPLACEMENT_CHAR);
    }

}