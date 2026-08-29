#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_data.h"
#include "evt/event_log_handle.h"
#include "utl/user_cache.h"


namespace wlf::evt
{
    enum class RenderStatus {
        Success,
        Overflow,
        Failed,
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
         * @param user_cache A reference to the global users cache.
         * 
         * @throws std::event_error If the underlying EVT render context cannot
         * be created.
        */
        EventRenderer() = delete;
        explicit EventRenderer(const BufferSize& buffer_size, utl::UserCache& user_cache);
        ~EventRenderer() = default;

        /**
         * Extracts system properties and XML from the provided event handle.
         *
         * @param elh Event Log Handle to render.
         * @param ed Data structure populated with values and ephemeral pointers
         *           to the extracted data.
         * 
         * @return A rendering status.
         *   Success : Event was rendered into the EventData structure (at least the system section)
         *   Overflow: The rendering of the system failed due to a buffer overflow.
         *   Failed  : The rendering of the system section failed for another reason.
        */
        RenderStatus render_event(const EventLogHandle& elh, EventData& ed) noexcept;

    private:
        const EventRenderContext _render_context;
        utl::UserCache& _user_cache;

        // Buffer holding referenced data
        RenderingBuffer _values_buffer;
        utl::UserAccountInfoPtr _account_buffer;
        RenderingBuffer _xml_buffer;

        // Renders the system properties into the variant buffer.
        RenderStatus render_system_data(const EventLogHandle& elh, SystemData& sd) noexcept;

        // Renders the user account
        void render_user_account(const ::PSID user_id, AccountData& ad) noexcept;

        // Renders the event as XML format
        bool render_event_xml(const EventLogHandle& elh, pugi::xml_document& xml_data) noexcept;
    };

}