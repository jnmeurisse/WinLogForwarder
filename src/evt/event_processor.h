#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_formatter.h"
#include "evt/event_filter.h"
#include "evt/event_log_handle.h"
#include "evt/event_message.h"
#include "evt/event_renderer.h"
#include "evt/event_util.h"
#include "utl/user_cache.h"


namespace wlf::evt {

    enum class ProcessResult {
        Success,
        Filtered,
        Failed
    };
    
    /**
     * Processes Windows Event Log records by rendering, filtering, and formatting them.
     *
     * Warning Because the internal Renderer maintains shared state buffers, this class
     *         is NOT thread-safe. Do not call `process()` concurrently from multiple 
     *         threads on the same instance.
    */
    class EventProcessor {
    public:
        struct Config {
            EventRenderer::BufferSize rendering_buffer_size;
            utl::UserCache& user_cache;
            EventIdFilter selected_event_id;
        };


        struct Context {
            ::DWORD max_message_size;
            IEventFilter filter;
            //IEventTransformer rewriter;
            IEventFormatter formatter;

        };

        EventProcessor() = delete;

        /**
         * Constructs an EventProcessor.
         * 
         * @throws std::event_error If the underlying EVT render context cannot be created.
        */
        explicit EventProcessor(const Config& config);
        ~EventProcessor() = default;

        /**
         * Renders, filters, and formats a single Windows Event.
         *
         * @param event_handle The handle to the Windows Event Log.
         * @param message On output, populated with formatted data if successful.
         * @return true If the event was successfully rendered.
        */
        ProcessResult process(const Context &ctx, const EventLogHandle& event_handle, EventMessageBuilder& emb) noexcept;

    private:
        EventRenderer _renderer;
    };

}
