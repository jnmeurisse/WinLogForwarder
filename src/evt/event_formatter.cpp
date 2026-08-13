#include "event_formatter.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

#include "evt/event_util.h"
#include "utl/pugixml.hpp"


namespace wlf::evt {

    // Replacement character for non PRINTUSASCII character
    constexpr wchar_t INVALID_ASCII = L'_';

    // RFC5424 NIL value
    constexpr char8_t NILVALUE = u8'-';

    // Maximum length of PRINTUSASCII fields
    constexpr size_t HOSTNAME_LENGTH = 255;
    constexpr size_t APP_NAME_LENGTH = 48;
    constexpr size_t PROCID_LENGTH = 128;
    constexpr size_t MSGID_LENGTH = 32;
    constexpr size_t SD_NAME_LENGTH = 32;
    constexpr size_t MAX_ASCII_FIELD_LENGTH = 512;

    // SD-NAME = 1*32PRINTUSASCII
    //              ; except '=', SP, ']', %d34(")
    constexpr const wchar_t* SD_NAME_EXCLUDE_CHARS = L"= ]\"";


    // Forward declaration
    static bool append_ascii(EventMessageBuilder& emb, const wchar_t* str, size_t count, const wchar_t* excluded, wchar_t replacement) noexcept;
    static bool append_ascii_value(EventMessageBuilder& emb, const wchar_t* str, size_t count) noexcept;

    /**
     * A RFC5424 Structured Data Parameter Name containing only USASCII characters.
     * This class can be used to store parameter names that are valid.
    */
    class SDAsciiParamName final : public SDParamName
    {
    public:
        SDAsciiParamName(const char8_t* name) noexcept : _name(name) {}

        bool append(EventMessageBuilder& emb) const noexcept override
        {
            return emb.write_chars(_name, std::strlen((const char* const)_name));
        }

        virtual explicit operator bool() const noexcept override
        {
            return _name && _name[0] != u8'\0';
        }

    private:
        const char8_t* const _name;
    };


    /**
     * A RFC5424 Structured Data Parameter Name containing any characters.
     * This class MUST be used to store parameter names that have not be validated.
    */
    class SDWcharParamName : public SDParamName
    {
    public:
        SDWcharParamName(const wchar_t* name) noexcept : _name(name) {}

        bool append(EventMessageBuilder& emb) const noexcept override
        {
            // Invalid characters are replaced by '_'
            //
            return append_ascii(emb, _name, SD_NAME_LENGTH, SD_NAME_EXCLUDE_CHARS, INVALID_ASCII);
        }

        virtual explicit operator bool() const noexcept override
        {
            return _name && _name[0] != L'\0';
        }

    private:
        const wchar_t* const _name;
    };


    // System Property Names 
    static const SDAsciiParamName PROVIDER_NAME(EventSystemPropertyName(EventSystemProperty::ProviderName));
    static const SDAsciiParamName PROVIDER_GUID(EventSystemPropertyName(EventSystemProperty::ProviderGuid));
    static const SDAsciiParamName EVENT_ID(EventSystemPropertyName(EventSystemProperty::EventID));
    static const SDAsciiParamName QUALIFIERS(EventSystemPropertyName(EventSystemProperty::Qualifiers));
    static const SDAsciiParamName LEVEL(EventSystemPropertyName(EventSystemProperty::Level));
    static const SDAsciiParamName TASK(EventSystemPropertyName(EventSystemProperty::Task));
    static const SDAsciiParamName OPCODE(EventSystemPropertyName(EventSystemProperty::Opcode));
    static const SDAsciiParamName KEYWORDS(EventSystemPropertyName(EventSystemProperty::Keywords));
    static const SDAsciiParamName TIME_CREATED(EventSystemPropertyName(EventSystemProperty::TimeCreated));
    static const SDAsciiParamName EVENT_RECORD_ID(EventSystemPropertyName(EventSystemProperty::EventRecordID));
    static const SDAsciiParamName ACTIVITY_ID(EventSystemPropertyName(EventSystemProperty::ActivityID));
    static const SDAsciiParamName RELATED_ACTIVITY_ID(EventSystemPropertyName(EventSystemProperty::RelatedActivityID));
    static const SDAsciiParamName PROCESS_ID(EventSystemPropertyName(EventSystemProperty::ProcessID));
    static const SDAsciiParamName THREAD_ID(EventSystemPropertyName(EventSystemProperty::ThreadID));
    static const SDAsciiParamName CHANNEL(EventSystemPropertyName(EventSystemProperty::Channel));
    static const SDAsciiParamName COMPUTER(EventSystemPropertyName(EventSystemProperty::Computer));
    static const SDAsciiParamName ACCOUNT_NAME(EventSystemPropertyName(EventSystemProperty::AccountName));
    static const SDAsciiParamName DOMAIN_NAME(EventSystemPropertyName(EventSystemProperty::Domain));
    static const SDAsciiParamName ACCOUNT_TYPE(EventSystemPropertyName(EventSystemProperty::AccountType));
    static const SDAsciiParamName VERSION(EventSystemPropertyName(EventSystemProperty::Version));


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


    static inline bool is_ascii_print(wchar_t wch)
    {
        //
        // PRINTUSASCII = %d33-126
        //
        return (wch >= 33) && (wch <= 127);
    }


    RFC5424EventFormatter::RFC5424EventFormatter(const Options& options) noexcept
        : _options(options)
    {
    }


    bool RFC5424EventFormatter::format(const EventData& event_data, EventMessageBuilder& emb) const noexcept
    {
        const bool format_status =
            // 1. HEADER
            append_header(emb, event_data) &&

            // 2. STRUCTURED-DATA
            append_sd(emb, event_data);

        // 3. Try to append UserData if present and enough space (RFC 5424 Payload)
        append_user_data(emb, event_data);

        return format_status;
    }


    bool RFC5424EventFormatter::append_header(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept
    {
        const unsigned int severity = level_to_syslog_severity(event_data.system_data.level);
        const unsigned int priority = (_options.facility * 8) + severity;

        return
            // 1. <PRI> and VERSION
            append_priority(emb, priority) &&
            append_version(emb, 1) &&
            append_space(emb) &&

            // 2. TIMESTAMP of the formatted event data
            append_timestamp(emb) &&
            append_space(emb) &&

            // 3. HOSTNAME
            append_ascii_value(emb, _options.hostname.c_str(), HOSTNAME_LENGTH) &&
            append_space(emb) &&

            // 4. APP-NAME
            append_ascii_value(emb, _options.appname.c_str(), APP_NAME_LENGTH) &&
            append_space(emb) &&

            // 5. PROCID
            append_id(emb, ::GetCurrentProcessId(), PROCID_LENGTH) &&
            append_space(emb) &&

            // 6. MSGID :: TODO
            append_id(emb, 0, MSGID_LENGTH);
    }


    inline bool RFC5424EventFormatter::append_space(EventMessageBuilder& emb) const noexcept
    {
        //
        // SP = %d32
        //
        return emb.append(u8' ');
    }


    bool RFC5424EventFormatter::append_priority(EventMessageBuilder& emb, unsigned int priority) const noexcept
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


    bool RFC5424EventFormatter::append_version(EventMessageBuilder& emb, unsigned int version) const noexcept
    {
        //
        // VERSION = NONZERO - DIGIT 0 * 2DIGIT
        //
        assert(version > 0 && version <= 99);

        return emb.append(version, 2);
    }


    bool RFC5424EventFormatter::append_timestamp(EventMessageBuilder& emb) const noexcept
    {
        SYSTEMTIME now;
        ::GetSystemTime(&now);

        return emb.append(now);
    }


    bool RFC5424EventFormatter::append_id(EventMessageBuilder& emb, unsigned int id, size_t count) const noexcept
    {
        return emb.append(id, count);
    }


    bool evt::RFC5424EventFormatter::append_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        //TODO add identity (list of IP address)
        return append_system_sd(emb, event_data)
            && append_event_sd(emb, event_data);
    }


    bool RFC5424EventFormatter::append_system_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        if (emb.free_space() < _options.sd_id_system.length() + 2)
            return false;

        if (!emb.append(u8'['))
            return false;

        if (!append_ascii(emb, _options.sd_id_system.c_str(), SD_NAME_LENGTH, SD_NAME_EXCLUDE_CHARS, INVALID_ASCII))
            return false;

        const system_data_t& system_data = event_data.system_data;
        const account_data_t& account_data = event_data.account_data;
        const EventSystemProperties& selected_properties = _options.selected_properties;

        if (selected_properties.contains(EventSystemProperty::ProviderName) && system_data.provider_name)
            append_sd_param(emb, PROVIDER_NAME, system_data.provider_name, _options.sd_value_max_length);

        if (selected_properties.contains(EventSystemProperty::ProviderGuid) && system_data.provider_guid)
            append_sd_param(emb, PROVIDER_GUID, system_data.provider_guid);

        if (selected_properties.contains(EventSystemProperty::EventID) && system_data.event_id)
            append_sd_param(emb, EVENT_ID, system_data.event_id.value());

        if (selected_properties.contains(EventSystemProperty::Qualifiers) && system_data.qualifiers)
            append_sd_param(emb, QUALIFIERS, system_data.qualifiers.value());

        if (selected_properties.contains(EventSystemProperty::Version) && system_data.version)
            append_sd_param(emb, VERSION, system_data.version.value());

        if (selected_properties.contains(EventSystemProperty::Level) && system_data.level)
            append_sd_param(emb, LEVEL, system_data.level.value());

        if (selected_properties.contains(EventSystemProperty::Task) && system_data.task)
            append_sd_param(emb, TASK, system_data.task.value());

        if (selected_properties.contains(EventSystemProperty::Opcode) && system_data.opcode)
            append_sd_param(emb, OPCODE, system_data.opcode.value());

        if (selected_properties.contains(EventSystemProperty::Keywords) && system_data.keywords)
            append_sd_param(emb, KEYWORDS, system_data.keywords.value());

        if (selected_properties.contains(EventSystemProperty::TimeCreated))
            append_sd_param(emb, TIME_CREATED, system_data.time_created);

        if (selected_properties.contains(EventSystemProperty::EventRecordID) && system_data.event_record_id)
            append_sd_param(emb, EVENT_RECORD_ID, system_data.event_record_id.value());

        if (selected_properties.contains(EventSystemProperty::ActivityID) && system_data.activity_id)
            append_sd_param(emb, ACTIVITY_ID, system_data.activity_id);

        if (selected_properties.contains(EventSystemProperty::RelatedActivityID) && system_data.related_activity_id)
            append_sd_param(emb, RELATED_ACTIVITY_ID, system_data.related_activity_id);

        if (selected_properties.contains(EventSystemProperty::ProcessID) && system_data.process_id)
            append_sd_param(emb, PROCESS_ID, system_data.process_id.value());

        if (selected_properties.contains(EventSystemProperty::ThreadID) && system_data.thread_id)
            append_sd_param(emb, THREAD_ID, system_data.thread_id.value());

        if (selected_properties.contains(EventSystemProperty::Channel) && system_data.channel)
            append_sd_param(emb, CHANNEL, system_data.channel, _options.sd_value_max_length);

        if (selected_properties.contains(EventSystemProperty::Computer) && system_data.computer)
            append_sd_param(emb, COMPUTER, system_data.computer, _options.sd_value_max_length);

        if (system_data.user_id) {
            if (selected_properties.contains(EventSystemProperty::AccountName) && account_data.name)
                append_sd_param(emb, ACCOUNT_NAME, account_data.name, _options.sd_value_max_length);

            if (selected_properties.contains(EventSystemProperty::Domain) && account_data.domain)
                append_sd_param(emb, DOMAIN_NAME, account_data.domain, _options.sd_value_max_length);

            if (selected_properties.contains(EventSystemProperty::AccountType) && account_data.type)
                append_sd_param(emb, ACCOUNT_TYPE, account_data.type, _options.sd_value_max_length);
        }

        return emb.append(u8']');
    }


    bool RFC5424EventFormatter::append_event_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        using namespace pugi;

        if (emb.free_space() < _options.sd_id_event.length() + 2)
            return true;

        xml_node event_node = event_data.xml_doc.first_child();
        if (!event_node || std::wcscmp(event_node.name(), L"Event") != 0)
            return true;

        // A lambda that checks if a node name = "EventData"
        auto is_event_data_node = [](xml_node& node) {
            return std::wcscmp(node.name(), L"EventData") == 0;
        };

        xml_node event_data_node = event_node.find_child(is_event_data_node);
        if (!event_data_node)
            return true;

        if (!emb.append(u8'['))
            return false;

        if (!append_ascii(emb, _options.sd_id_event.c_str(), SD_NAME_LENGTH, SD_NAME_EXCLUDE_CHARS, INVALID_ASCII))
            return false;

        for (xml_node node : event_data_node) {
            if (std::wcscmp(node.name(), L"Data") !=0)
                continue;

            const wchar_t* param_name = L"data";
            xml_attribute attribute = node.attribute(L"Name");
            if (attribute)
                param_name = attribute.value();

            append_sd_param(emb, SDWcharParamName(param_name), node.text().as_string(nullptr), _options.sd_value_max_length);
        }

        return emb.append(u8']');
    }

    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const wchar_t* value, size_t max_chars) const noexcept
    {
        if (!name || !value || *value == L'\0')
            return true;

        auto guard = emb.savepoint();

        const bool append_status = append_sd_name(emb, name)
            && emb.append(u8"=\"")
            && append_sd_value(emb, value, max_chars)
            && emb.append(u8'"');

        return guard.commit(1);
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const::FILETIME& ft) const noexcept
    {
        if (!name)
            return true;

        ::SYSTEMTIME system_time;
        if (!::FileTimeToSystemTime(&ft, &system_time))
            return true;

        auto guard = emb.savepoint();

        const bool append_status = append_sd_name(emb, name)
            && emb.append(u8"=\"")
            && emb.append(system_time)
            && emb.append(u8'"');

        return guard.commit(1);
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const SDParamName& name, uint64_t value) const noexcept
    {
        if (!name)
            return true;

        auto guard = emb.savepoint();

        const bool append_status = append_sd_name(emb, name)
            && emb.append(u8"=\"")
            && emb.append(value, std::numeric_limits<size_t>::digits10)
            && emb.append(u8'"');

        return guard.commit(1);
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const::GUID* value) const noexcept
    {
        if (!name)
            return true;

        auto guard = emb.savepoint();

        const bool append_status = append_sd_name(emb, name)
            && emb.append(u8"=\"")
            && emb.append(value)
            && emb.append(u8'"');

        return guard.commit(1);
    }


    bool RFC5424EventFormatter::append_sd_name(EventMessageBuilder& emb, const SDParamName& name) const noexcept
    {
        return name.append(emb);
    }


    bool RFC5424EventFormatter::append_sd_value(EventMessageBuilder& emb, const wchar_t* const value, size_t max_chars) const noexcept
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

        while (*p && max_chars > 0 && emb_append_ok) {
            // Copy characters from the source string to a buffer until
            // the buffer is full.  Buffer is considered as full when the
            // filling index is greater than the buffer size.  The buffer
            // has twice the capacity so that we have always enough room
            // to store surrogate or escaped characters.
            constexpr size_t buffer_size = 1024;
            std::array<wchar_t, 2 * buffer_size> buffer;

            // index in the output buffer
            size_t j = 0;

            while (*p && max_chars > 0 && j < buffer_size) {
                const wchar_t wch = *p++;

                if (IS_HIGH_SURROGATE(wch)) {
                    buffer[j++] = wch;
                    if (*p && IS_LOW_SURROGATE(*p))
                        buffer[j++] = *p++;
                }
                else {
                    if (needs_escape(wch))
                        buffer[j++] = L'\\';

                    buffer[j++] = wch;
                }

                --max_chars;
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

        virtual void write(const void* data, size_t size)
        {
            overflow |= !_emb.write_chars(static_cast<const wchar_t*>(data), size / sizeof(wchar_t));
        }
    };


    void RFC5424EventFormatter::append_user_data(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        using namespace pugi;

        if (event_data.xml_doc.empty())
            return;

        xml_node event_node = event_data.xml_doc.first_child();
        if (!event_node || std::wcscmp(event_node.name(), L"Event") != 0)
            return;

        // A lambda that checks if a node name = "UserData"
        auto is_event_user_node = [](xml_node& node) {
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

        savepoint.commit(0);
        return;
    }


    static bool append_ascii(EventMessageBuilder& emb, const wchar_t* str, size_t count, const wchar_t* excluded, wchar_t replacement) noexcept
    {
        if (!str)
            return true;

        // Temporary buffer holding the transformed string
        std::array<wchar_t, MAX_ASCII_FIELD_LENGTH> buffer;
        assert(count <= buffer.size());
        
        // index in the buffer
        size_t i = 0;

        while (*str && i < std::min(count, buffer.size())) {
            wchar_t wch = *str++;

            if (IS_HIGH_SURROGATE(wch)) {
                wch = replacement;
                if (*str && IS_LOW_SURROGATE(*str))
                    ++str;
            }
            else if (!is_ascii_print(wch))
                wch = replacement;
            else if (excluded && std::wcschr(excluded, wch))
                wch = replacement;

            buffer[i++] = wch;
        }

        return emb.write_chars(buffer.data(), i);
    }


    static bool append_ascii_value(EventMessageBuilder& emb, const wchar_t* str, size_t count) noexcept
    {
        if (!str)
            return emb.append(NILVALUE);
        return append_ascii(emb, str, count, nullptr, INVALID_ASCII);
    }

}