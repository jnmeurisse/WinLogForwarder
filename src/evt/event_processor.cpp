#include "event_processor.h"


namespace wlf::evt {

	EventProcessor::EventProcessor(EventRenderer& renderer)
		: _renderer(renderer)
	{
	}


	bool EventProcessor::process(const Context& ctx, const EventLogHandle& event_handle, EventMessageBuilder& message) noexcept
	{
		// event_data acts as an ephemeral view into system properties stored into
		// the renderer memory.
		EventData event_data;

		// Extract system and xml data from the event
		if (!_renderer.render_event(event_handle, event_data)) {
			return false; // Failed to extract data
		}

		// Apply filtering logic if set
		if (ctx.filter && !ctx.filter->filter(event_data, message)) {
			return false; // Event is rejected
		}

		// Apply rewriter logic if set
		//if (ctx.rewriter && !ctx.rewriter->transform(event_data, message))
		//	return false;	// Failed to rewrite

		// Apply formatting logic if set.  
		// Return false is no formatter or if format failed
		//return ctx.formatter && ctx.formatter->format(event_data, message);
        return false;
	}

}
