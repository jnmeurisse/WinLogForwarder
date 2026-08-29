#include "event_renderer.h"

#include "evt/event_variants.h"
#include "evt/event_log_handle.h"
#include "utl/exception.h"


namespace wlf::evt {


    EventRenderer::EventRenderer(const BufferSize& buffer_size, utl::UserCache& user_cache)
        : _render_context(EventRenderContext::create_system_context())
        , _user_cache(user_cache)
        , _values_buffer(buffer_size.system_data_buffer_size_limit)
        , _xml_buffer(buffer_size.event_data_buffer_size_limit)
    {
    }


    RenderStatus EventRenderer::render_event(const EventLogHandle& elh, EventData& ed) noexcept
    {
        const RenderStatus status = render_system_data(elh, ed.system_data);

        if (status == RenderStatus::Success) {
            // Get the account name from the user ID if available
            render_user_account(ed.system_data.user_id, ed.account_data);

            // Get the xml data.  We do not consider the returned status.  In 
            // case of rendering error, we do not return the xml message.
            if (!render_event_xml(elh, ed.xml_doc))
                ed.xml_doc.reset();
        }

        return status;
    }


    RenderStatus EventRenderer::render_system_data(const EventLogHandle& elh, SystemData& sd) noexcept
    {
        ::DWORD property_count = elh.render_values(_render_context, _values_buffer);
        if (property_count == 0) {
            return ::GetLastError() == ERROR_BUFFER_OVERFLOW
                ? RenderStatus::Overflow
                : RenderStatus::Failed;
        }

        // Extract log information from the buffer
        EventVariants variants(_values_buffer.data(), property_count);

        // Load all other properties
        sd.provider_name = variants.get_string(EvtSystemProviderName);
        sd.provider_guid = variants.get_guid(EvtSystemProviderGuid);
        sd.event_id = variants.get_uint16(EvtSystemEventID);
        sd.qualifiers = variants.get_uint16(EvtSystemQualifiers);
        sd.level = variants.get_byte(EvtSystemLevel);
        sd.task = variants.get_uint16(EvtSystemTask);
        sd.opcode = variants.get_byte(EvtSystemOpcode);
        sd.keywords = variants.get_uint64(EvtSystemKeywords);
        sd.time_created = variants.get_time(EvtSystemTimeCreated);
        sd.event_record_id = variants.get_uint64(EvtSystemEventRecordId);
        sd.activity_id = variants.get_guid(EvtSystemActivityID);
        sd.related_activity_id = variants.get_guid(EvtSystemRelatedActivityID);
        sd.process_id = variants.get_uint32(EvtSystemProcessID);
        sd.thread_id = variants.get_uint32(EvtSystemThreadID);
        sd.channel = variants.get_string(EvtSystemChannel);
        sd.computer = variants.get_string(EvtSystemComputer);
        sd.user_id = variants.get_sid(EvtSystemUserID);
        sd.version = variants.get_byte(EvtSystemVersion);

        return RenderStatus::Success;
    }


    static const wchar_t* sid_name_use_to_string(const SID_NAME_USE sid_name_use) noexcept {
        switch (sid_name_use) {
        case SidTypeUser:           return L"User";
        case SidTypeGroup:          return L"Group";
        case SidTypeDomain:         return L"Domain";
        case SidTypeAlias:          return L"Alias";
        case SidTypeWellKnownGroup: return L"WellKnownGroup";
        case SidTypeDeletedAccount: return L"DeletedAccount";
        case SidTypeInvalid:        return L"Invalid";
        case SidTypeUnknown:        return L"Unknown";
        case SidTypeComputer:       return L"Computer";
        case SidTypeLabel:          return L"Label";
        case SidTypeLogonSession:   return L"LogonSession";
        default:                    return L"Unknown";
        }
    }


    void EventRenderer::render_user_account(const ::PSID user_id, AccountData& account_data) noexcept
    {
        _account_buffer.reset();

        try {
            utl::UserSID user_sid(user_id);

            if (user_sid.valid()) {
                _account_buffer = _user_cache.get_account_info(user_sid);
                if (_account_buffer) {
                    account_data.name = _account_buffer->user_name.data();
                    account_data.domain = _account_buffer->domain.data();
                    account_data.type = sid_name_use_to_string(_account_buffer->account_type);
                }
            }
        }
        catch (const utl::os_error&) {
        }
    }


    bool EventRenderer::render_event_xml(const EventLogHandle& elh, pugi::xml_document& xml_data) noexcept
    {
        // Render in a temporary buffer
        const ::DWORD buffer_size = elh.render_xml(_xml_buffer);
        if (buffer_size == 0)
            return false;

        // Load the XML document
        const pugi::xml_parse_result result = xml_data.load_buffer_inplace(
            _xml_buffer.data(),
            buffer_size,
            pugi::parse_default,
            pugi::xml_encoding::encoding_wchar
        );

        return result.status == pugi::status_ok || result.status == pugi::status_no_document_element;
    }

}