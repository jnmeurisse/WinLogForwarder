#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "evt/event_data.h"
#include "evt/event_filter.h"
#include "evt/event_log_handle.h"
#include "evt/event_message.h"
#include "evt/event_renderer.h"


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
        explicit EventProcessor(EventRenderer& renderer);
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
        EventRenderer& _renderer;
    };

}
