#include "event_formatter.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
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

    constexpr size_t MAX_SD_VALUE_LENGTH = 4096;


    /**
     * Helper function to map a Windows Event Log level to a Syslog severity.
     * Windows: 1=Critical, 2=Error, 3=Warning, 4=Info, 5=Debug
     * Syslog: 0=Emergency ... 7=Debug
     */
    static inline unsigned int MapWinLevelToSyslogSeverity(uint8_t level) 
    {
        switch (level) {
        case 1: return 2; // Critical
        case 2: return 3; // Error
        case 3: return 4; // Warning
        case 4: return 6; // Informational
        case 5: return 7; // Debug
        default: return 6; // Default to Info
        }
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


    bool RFC5424EventFormatter::format(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        return
            // 1. HEADER
            append_header(emb, event_data) &&

            // 1. STRUCTURED-DATA
            append_sd(emb, event_data) &&

            // 8. MSG (RFC 5424 Payload)
            append_msg(emb, event_data);
    }



    bool RFC5424EventFormatter::append_header(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept
    {
        const unsigned int severity = MapWinLevelToSyslogSeverity(event_data.level);
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
            append_ascii(emb, _options.hostname.c_str(), HOSTNAME_LENGTH, nullptr, INVALID_ASCII) &&
            append_space(emb) &&

            // 4. APP-NAME
            append_ascii(emb, event_data.provider_name, APP_NAME_LENGTH, nullptr, INVALID_ASCII) &&
            append_space(emb) &&

            // 5. PROCID
            append_id(emb, ::GetCurrentProcessId(), PROCID_LENGTH) &&
            append_space(emb) &&

            // 6. MSGID
            append_id(emb, event_data.event_id, MSGID_LENGTH);
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


    bool RFC5424EventFormatter::append_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept
    {
        if (!emb.append(u8'['))
            return false;

        append_sd_param(emb, L"EventTime", event_data.event_time);
        append_sd_param(emb, L"EventRecordID", event_data.event_record_id);
        append_sd_param(emb, L"User", event_data.username);
        append_sd_param(emb, L"Domain", event_data.domain);
        append_sd_param(emb, L"AccountType", event_data.account_type);
        append_sd_param(emb, L"Channel", event_data.channel);
        append_sd_param(emb, L"Computer", event_data.computer);
        append_sd_param(emb, event_data.xml_doc);
        
        return emb.append(u8']');
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const wchar_t* name, const wchar_t* value) noexcept
    {
        if (!name || *name == L'\0' || !value || *value == L'\0')
            return true;

        // Allocate a temporary buffer.
        // Explain why -5 !!
        // due to alignment of wchar to utf8 translation (+4)
        // and space for ]
        EventMessageBuilder sd_buffer(emb.free_space() - 5);

        return append_sd_name(sd_buffer, name) 
            && sd_buffer.append(u8"=\"")
            && append_sd_value(sd_buffer, value, MAX_SD_VALUE_LENGTH)
            && sd_buffer.append(u8'"')
            && emb.append(sd_buffer);
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const wchar_t* name, const::FILETIME& ft) noexcept
    {
        if (!name || *name == L'\0')
            return true;

        ::SYSTEMTIME system_time;
        if (!::FileTimeToSystemTime(&ft, &system_time))
            return true;

        EventMessageBuilder sd_buffer(emb.free_space() - 4);
        return append_sd_name(sd_buffer, name)
            && sd_buffer.append(u8"=\"")
            && sd_buffer.append(system_time)
            && sd_buffer.append(u8'"')
            && emb.append(sd_buffer);
    }


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, const wchar_t* name, uint64_t value) noexcept
    {
        if (!name || *name == L'\0')
            return true;

        EventMessageBuilder sd_buffer(emb.free_space() - 4);
        return append_sd_name(sd_buffer, name)
            && sd_buffer.append(u8"=\"")
            && sd_buffer.append(value, 99999999999999)
            && sd_buffer.append(u8'"')
            && emb.append(sd_buffer);
    }


    struct sd_param_walker : pugi::xml_tree_walker
    {
        EventMessageBuilder& emb;
        
        sd_param_walker(EventMessageBuilder& emb) noexcept
            : pugi::xml_tree_walker()
            , emb(emb)
        {
        }

        virtual bool for_each(pugi::xml_node& node)
        {
            if (std::strcmp(node.name(), "Keywords") == 0) {

                // Build the path manually by climbing up the parent chain
                std::string path = "";
                pugi::xml_node current = node;

                while (current && !current.parent().empty() && current.type() != pugi::node_document) {
                    path = "/" + std::string(current.name()) + path;
                    current = current.parent();
                }

                if (path.empty()) {
                    path = "/" + std::string(node.name());
                }

                // Modify text as originally intended
                node.text().set(node.text().as_string(), 10);
            }

            return true;
        }
    };


    bool RFC5424EventFormatter::append_sd_param(EventMessageBuilder& emb, pugi::xml_document& xml_doc) noexcept
    {
        using namespace pugi;

        xml_node event_node = xml_doc.first_child();
        if (!event_node || event_node.name() != L"Event")
            return true;

        // A lambda that checks if a node name is System
        auto is_event_data_node = [](xml_node& node) {
            return node.name() == L"EventData";
        };

        xml_node event_data_node = event_node.find_child(is_event_data_node);
        if (!event_data_node)
            return true;

        for (xml_node node : event_data_node) {
            if (node.name() != L"Data")
                continue;

            xml_attribute attribute = node.attribute(L"Name");
            if (!attribute)
                continue;

            append_sd_param(emb, attribute.value(), node.value());
        }
    }


    bool RFC5424EventFormatter::append_sd_name(EventMessageBuilder& emb, const wchar_t* const name) noexcept
    {
        //
        // SD-NAME = 1*32PRINTUSASCII
        //              ; except '=', SP, ']', %d34(")
        //
        // Invalid characters are replaced by '_'
        //
        return append_ascii(emb, name, SD_NAME_LENGTH, L"= ]\"", INVALID_ASCII);
    }


    bool RFC5424EventFormatter::append_sd_value(EventMessageBuilder& emb, const wchar_t* const value, size_t count) noexcept
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

        while (*p && count > 0 && emb_append_ok) {
            // Copy characters from the source string to a buffer until
            // the buffer is full.  Buffer is considered as full when the
            // filling index is greater than the buffer size.  The buffer
            // has twice the capacity so that we have always enough room
            // to store surrogate or escaped characters.
            constexpr size_t buffer_size = 1024;
            std::array<wchar_t, 2 * buffer_size> buffer;

            // index in the output buffer
            size_t j = 0;

            while (*p && count > 0 && j < buffer_size) {
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

                --count;
            }

            if (j > 0)
                emb_append_ok = emb.write_chars(buffer.data(), j);
        }

        return emb_append_ok;
    }


    struct xml_string_writer : pugi::xml_writer
    {
        EventMessageBuilder message;
        bool overflow = false;
 
        xml_string_writer(EventMessageBuilder& message) noexcept : message(message) {};

        virtual void write(const void* data, size_t size)
        {
            overflow |= !message.append_chars(static_cast<const char*>(data), size, false));
        }
    };



    bool RFC5424EventFormatter::append_xml(EventMessageBuilder& message, const EventData& event_data) const noexcept
    {
        if (!event_data.xml_doc.empty())
            return true;

        pugi::xml_document xml_doc;
        xml_doc.load_buffer_inplace(
            event_data.xml, 
            std::wcslen(event_data.xml) * sizeof(wchar_t),
            pugi::parse_default, 
            pugi::xml_encoding::encoding_wchar);
       
        simple_walker walker;
        xml_doc.traverse(walker);

        xml_string_writer writer(message);
        xml_doc.save(writer, "", pugi::format_raw, pugi::xml_encoding::encoding_utf8);

        return !writer.overflow;
    }


    bool RFC5424EventFormatter::append_ascii(EventMessageBuilder& emb, const wchar_t* str, size_t count, const wchar_t* excluded, wchar_t replacement) noexcept
    {
        if (!str)
            return emb.append(NILVALUE);

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
                    *str++;
            }
            else if (!is_ascii_print(wch))
                wch = replacement;
            else if (excluded && std::wcschr(excluded, wch))
                wch = replacement;

            buffer[i++] = wch;
        }

        return emb.write_chars(buffer.data(), i);
    }


}