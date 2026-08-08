#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_data.h"
#include "evt/event_filter.h"
#include "evt/event_log_handle.h"
#include "evt/event_message.h"
#include "utl/user_cache.h"


namespace wlf::evt {
	
	using namespace wlf;


    /**
     * Processes Windows Event Log records by rendering, filtering, and formatting them.
     *
     * Warning Because the internal Renderer maintains shared state buffers, this class
     *         is NOT thread-safe. Do not call `process()` concurrently from multiple 
     *         threads on the same instance.
    */
    class EventProcessor {
    public:
        struct Context {
            IEventFilter filter;
            //IEventTransformer rewriter;
            //IEventFormatter formatter;
        };


        EventProcessor() = delete;

        /**
         * Constructs an EventProcessor.
         * 
         * @throws std::runtime_error If the underlying EVT render context cannot be created.
        */
        explicit EventProcessor(utl::UserCache& user_cache);
        ~EventProcessor() = default;

        /**
         * Renders, filters, and formats a single Windows Event.
         *
         * @param event_handle The handle to the Windows Event Log.
         * @param message On output, populated with formatted data if successful.
         * @return true If the event was successfully rendered.
        */
        bool process(const Context& ctx, const EventLogHandle& event_handle, EventMessageBuilder& message) noexcept;

    private:
        /**
         * Handles raw extraction of system properties and XML message using
         * the Windows Event Log API.
        */
        class Renderer {
        public:
            /**
             * Initializes the renderer and creates the system rendering context.
             * 
             * @param user_cache Reference to the cache for resolving SIDs.
            */
            Renderer() = delete;
            explicit Renderer(utl::UserCache& user_cache);
            ~Renderer() = default;

            /**
             * Extracts system properties and XML from the provided event handle.
             * 
             * @param event_handle The event handle to render.
             * @param event_data Data structure populated with ephemeral pointers 
             * to the extracted data.
             * @return true if rendering succeeded, false otherwise.
            */
            bool render_event(const EventLogHandle& event_handle, EventData& event_data) noexcept;

        private:
            const EventLogHandle::RenderContext _render_system_context;
            utl::UserCache& _user_cache;

            // Buffer holding rendered data
            evt::RenderingBuffer _values_buffer;
            utl::UserAccountInfoPtr _account_buffer;
            evt::RenderingBuffer _xml_buffer;

            // Renders the system properties into the variant buffer.
            bool render_system_data(const EventLogHandle& event_handle, system_data_t& system_data) noexcept;

            // Renders the user account
            void render_user_account(const ::SID* user_id, account_data_t& account_data) noexcept;

            // Renders the event as XML format
            bool render_event_xml(const EventLogHandle& event_handle, pugi::xml_document& xml_data) noexcept;
        };

        Renderer _renderer;
    };

}
