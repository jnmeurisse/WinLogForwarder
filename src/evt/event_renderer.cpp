#include "event_renderer.h"

#include "evt/event_variants.h"


namespace wlf::evt {

    EventRenderer::EventRenderer(BufferSize buffer_size, utl::UserCache& user_cache, const EventIdFilter& filter)
        : _render_context(EventRenderContext::create_system_context())
        , _user_cache(user_cache)
        , _values_buffer(buffer_size.system_data_buffer_size_limit)
        , _xml_buffer(buffer_size.event_data_buffer_size_limit)
        , _id_filter(filter)
    {
    }


    RenderStatus EventRenderer::render_event(const EventLogHandle& event_handle, EventData& event_data) noexcept
    {
        const RenderStatus status = render_system_data(event_handle, event_data.system_data);

        if (status == RenderStatus::Success) {
            // Get the account name from the user ID if available
            render_user_account(event_data.system_data.user_id, event_data.account_data);

            // Get the xml data.  We do not consider the returned status.  In 
            // case of rendering error, we do not return the xml message.
            if (!render_event_xml(event_handle, event_data.xml_doc))
                event_data.xml_doc.reset();
        }

        return status;
    }


    RenderStatus EventRenderer::render_system_data(const EventLogHandle& event_handle, system_data_t& system_data) noexcept
    {
        system_data.clear();

        ::DWORD property_count = event_handle.render_values(_render_context, _values_buffer);
        if (property_count == 0)
            return RenderStatus::Failed;

        // Extract log information from the buffer
        EventVariants variants(_values_buffer.data(), property_count);

        // Check if this event ID is selected
        system_data.event_id = variants.get_uint16(EvtSystemEventID);
        if (_id_filter.size() > 0 && system_data.event_id.has_value()) {
            if (_id_filter.count(system_data.event_id.value()) == 0)
                return RenderStatus::Filtered;
        }

        system_data.provider_name = variants.get_string(EvtSystemProviderName);
        system_data.provider_guid = variants.get_guid(EvtSystemProviderGuid);
        system_data.qualifiers = variants.get_uint16(EvtSystemQualifiers);
        system_data.level = variants.get_byte(EvtSystemLevel);
        system_data.task = variants.get_uint16(EvtSystemTask);
        system_data.opcode = variants.get_byte(EvtSystemOpcode);
        system_data.keywords = variants.get_uint64(EvtSystemKeywords);
        system_data.time_created = variants.get_time(EvtSystemTimeCreated);
        system_data.event_record_id = variants.get_uint64(EvtSystemEventRecordId);
        system_data.activity_id = variants.get_guid(EvtSystemActivityID);
        system_data.related_activity_id = variants.get_guid(EvtSystemRelatedActivityID);
        system_data.process_id = variants.get_uint32(EvtSystemProcessID);
        system_data.thread_id = variants.get_uint32(EvtSystemThreadID);
        system_data.channel = variants.get_string(EvtSystemChannel);
        system_data.computer = variants.get_string(EvtSystemComputer);
        system_data.user_id = variants.get_sid(EvtSystemUserID);
        system_data.version = variants.get_byte(EvtSystemVersion);

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


    void EventRenderer::render_user_account(const ::PSID user_id, account_data_t& account_data) noexcept
    {
        account_data.clear();
        _account_buffer.reset();

        if (user_id)
            _account_buffer = _user_cache.get_account_info(user_id);

        if (_account_buffer)
        {
            account_data.name = _account_buffer->user_name.data();
            account_data.domain = _account_buffer->domain.data();
            account_data.type = sid_name_use_to_string(_account_buffer->account_type);
        }
    }


    bool EventRenderer::render_event_xml(const EventLogHandle& event_handle, pugi::xml_document& xml_data) noexcept
    {
        xml_data.reset();

        // Render in a temporary buffer
        if (!event_handle.render_xml(_xml_buffer))
            return false;

        // Load the XML document
        const pugi::xml_parse_result result = xml_data.load_buffer_inplace(
            _xml_buffer.data(),
            _xml_buffer.size(),
            pugi::parse_default,
            pugi::xml_encoding::encoding_wchar
        );

        return result.status == pugi::status_ok || result.status == pugi::status_no_document_element;
    }

}
