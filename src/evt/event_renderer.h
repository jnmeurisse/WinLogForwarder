#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_data.h"
#include "evt/event_log_handle.h"
#include "evt/event_util.h"
#include "utl/user_cache.h"


namespace wlf::evt
{
    enum class RenderStatus {
        Success,
        Filtered,
        Failed
    };

    /**
     * Handles raw extraction of system properties and XML message using
     * the Windows Event Log API.
     * 
     * This class is NOT thread-safe. 
     * 
    */
    class EventRenderer {
    public:
        struct BufferSize {
            ::DWORD system_data_buffer_size_limit;
            ::DWORD event_data_buffer_size_limit;
        };

        /**
         * Initializes the renderer and creates the system rendering context.
         *
         * @param buffer_size Maximum internal buffer size.
         * @param user_cache Reference to the cache for resolving SIDs.
         * @param filter Reference to a pre-filter list of event ID
         * 
         * @throws std::event_error If the underlying EVT render context cannot be created.
        */
        EventRenderer() = delete;
        explicit EventRenderer(BufferSize buffer_size, utl::UserCache& user_cache, const EventIdFilter& filter);
        ~EventRenderer() = default;

        /**
         * Extracts system properties and XML from the provided event handle.
         *
         * @param event_handle The event handle to render.
         * @param event_data Data structure populated with values and ephemeral pointers
         * to the extracted data.
         * 
         * @return A rendering status.
         *   Success : Event was rendered into the EventData structure (at least the system section)
         *   Filtered: Event ID is not selected.
         *   Failed  : The rendering of the system section failed.
        */
        RenderStatus render_event(const EventLogHandle& event_handle, EventData& event_data) noexcept;

    private:
        const EventRenderContext _render_context;

        // A reference to the user cache.  This cache is shared with other rendered.
        // UserCache is thread safe.
        utl::UserCache& _user_cache;

        // Buffer holding referenced data
        RenderingBuffer _values_buffer;
        utl::UserAccountInfoPtr _account_buffer;
        RenderingBuffer _xml_buffer;

        // A collection of EventID to render.  All if filter is empty
        const EventIdFilter _id_filter;

        // Renders the system properties into the variant buffer.
        RenderStatus render_system_data(const EventLogHandle& event_handle, system_data_t& system_data) noexcept;

        // Renders the user account
        void render_user_account(const ::PSID user_id, account_data_t& account_data) noexcept;

        // Renders the event as XML format
        bool render_event_xml(const EventLogHandle& event_handle, pugi::xml_document& xml_data) noexcept;
    };

}