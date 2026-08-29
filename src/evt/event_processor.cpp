#include "event_processor.h"

#include "evt/event_renderer.h"
#include "event_data.h"


namespace wlf::evt {

	EventProcessor::EventProcessor(const EventProcessorConfig& config)
        : _renderer(EventRenderer::BufferSize{
                config.system_data_buffer_size_limit,
                config.event_data_buffer_size_limit}, config.user_cache)
	{
	}


    ProcessResult EventProcessor::process(const EventProcessContext& ctx, const EventLogHandle& elh, EventMessageBuilder& emb) noexcept
	{
		// event_data acts as an ephemeral view into system properties stored into
		// the renderer memory.
		EventData event_data;

		// Extract system and xml data from the event
        const RenderStatus status = _renderer.render_event(elh, event_data);
        if (status == RenderStatus::Failed)
            // Failed to extract data
            return ProcessResult::Failed;

        else {
            // Apply filtering logic if set
            if (ctx.filter && ctx.filter->filter(event_data, emb))
                // Event is rejected
                return ProcessResult::Filtered;

            // Apply rewriter logic if set
            //if (!ctx.rewriter->transform(event_data, message))
            //	return false;	// Failed to rewrite

            // Apply formatting logic if set.  
            // Return false is no formatter or if format failed
            return ctx.formatter && ctx.formatter->format(event_data, emb)
                ? ProcessResult::Success
                : ProcessResult::Failed;
        }
	}

}
