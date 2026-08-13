#include "event_processor.h"


namespace wlf::evt {

	EventProcessor::EventProcessor(const Config& config)
        : _renderer(config.rendering_buffer_size, config.user_cache, config.selected_event_id)
	{
	}


    ProcessResult EventProcessor::process(const Context& ctx, const EventLogHandle& event_handle, EventMessageBuilder& emb) noexcept
	{
		// event_data acts as an ephemeral view into system properties stored into
		// the renderer memory.
		EventData event_data;

        RenderStatus status;

		// Extract system and xml data from the event
        status = _renderer.render_event(event_handle, event_data);
        if (status == RenderStatus::Failed)
            // Failed to extract data
            return ProcessResult::Failed;

        else if (status == RenderStatus::Filtered)
            // Event is not selected
            return ProcessResult::Filtered;

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
