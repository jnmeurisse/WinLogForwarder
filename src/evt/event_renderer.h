#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_data.h"
#include "evt/event_log_handle.h"
#include "utl/user_cache.h"


namespace wlf::evt
{
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
         * @param user_cache Reference to the cache for resolving SIDs.
        */
        EventRenderer() = delete;
        explicit EventRenderer(BufferSize buffer_size, utl::UserCache& user_cache);
        ~EventRenderer() = default;

        /**
         * Extracts system properties and XML from the provided event handle.
         *
         * @param event_handle The event handle to render.
         * @param event_data Data structure populated with values and ephemeral pointers
         * to the extracted data.
         * 
         * @return true if rendering succeeded, false otherwise.
        */
        bool render_event(const EventLogHandle& event_handle, EventData& event_data) noexcept;

    private:
        const EventRenderContext _render_context;

        // A reference to the user cache.  This cache is shared with other rendered.
        // UserCache is thread safe.
        utl::UserCache& _user_cache;

        // Buffer holding referenced data
        RenderingBuffer _values_buffer;
        utl::UserAccountInfoPtr _account_buffer;
        RenderingBuffer _xml_buffer;

        // Renders the system properties into the variant buffer.
        bool render_system_data(const EventLogHandle& event_handle, system_data_t& system_data) noexcept;

        // Renders the user account
        void render_user_account(const ::PSID user_id, account_data_t& account_data) noexcept;

        // Renders the event as XML format
        bool render_event_xml(const EventLogHandle& event_handle, pugi::xml_document& xml_data) noexcept;
    };

}