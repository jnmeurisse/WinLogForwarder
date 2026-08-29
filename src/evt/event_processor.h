#pragma once

#include "global.h"
#include "evt/event_filter.h"
#include "evt/event_formatter.h"
#include "evt/event_log_handle.h"
#include "evt/event_message.h"
#include "evt/event_renderer.h"
#include "evt/event_types.h"



namespace wlf::evt {

    enum class ProcessResult {
        Success,
        Filtered,
        Failed
    };


    struct EventProcessContext {
        const EventFilterPtr filter;
        const EventFormatterPtr formatter;

        inline size_t max_message_size() const noexcept {
            return formatter ? formatter->max_message_size() : MinMaxSyslogMsgLength;
        }
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
        EventProcessor() = delete;

        /**
         * Constructs an EventProcessor.
         * 
         * @throws std::event_error If the underlying EVT render context cannot be created.
        */
        explicit EventProcessor(const EventProcessorConfig& config);
        ~EventProcessor() = default;

        /**
         * Renders, filters, and formats a single Windows Event.
         *
         * @param event_handle The handle to the Windows Event Log.
         * @param message On output, populated with formatted data if successful.
         * @return true If the event was successfully rendered.
        */
        ProcessResult process(const EventProcessContext& ctx, const EventLogHandle& event_handle, EventMessageBuilder& emb) noexcept;

    private:
        EventRenderer _renderer;
    };

}
