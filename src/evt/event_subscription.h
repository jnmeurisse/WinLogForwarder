#pragma once

#include <functional>
#include <memory>
#include <string>

#include "evt/event_processor.h"
#include "evt/event_subscription_handle.h"
#include "utl/signal.h"


namespace wlf::evt {

	/**
	 * Callback function used to process fetched Event Logs.
	 * 
	 * @param event The parsed event instance.
	 * @return true if the event was processed successfully, false otherwise.
	 */
	using EventHandlerCallback = std::function<ProcessResult(const EventProcessor::Context& ctx, const EventLogHandle& event)>;


	/**
	 * Manages a pull-model subscription to the Windows Event Log.
	 *
	 * This class subscribes to future events on a specific channel matching an XPath query.
	 * It uses a signal to wake up and fetch new batches of events.
	 *
	 * @note This class is not thread-safe.
	 */
	class EventSubscription {
	public:
		/**
		 * Configuration options for the Event Log subscription.
		 */
		struct Config
		{
			// The event channel to subscribe to (e.g., L"Application").
			std::wstring channel;

            // 
            EventIdFilter selected_event_id;

			//
			EventProcessor::Context processing_context;
		};


		EventSubscription() = delete;

		/**
		 * Creates a new Event Log subscription.
		 * 
		 * @param options The channel and query configuration for the subscription.
		 */
		EventSubscription(const Config& options);
		
		/**
		 * Closes the underlying EVT_HANDLE.
		 */
		~EventSubscription() = default;

		/**
		 * Indicates the result of a drain operation.
		 */
		enum class DrainResult
		{
			Continue,   // more events are available; call drain again
			Completed,  // all events have been drained; wait for a signal to drain again
			Cancelled,  // The subscription was cancelled
			Failed, 	// An error occurred while fetching events.
            Filtered    // Event was rejected
		};

		/**
		 * Fetches pending events and processes them using the provided handler.
		 *
		 * If the subscription is already drained, it will check the underlying
		 * signal state before attempting a new fetch.
		 *
		 * @param handler The callback executed for each successfully fetched event.
		 * @return The status of the drain loop (Continue, Completed, Cancelled, or Failed).
		 */
		DrainResult drain(const EventHandlerCallback& handler) noexcept;

		/**
		 * Cancels the active event subscription.
		 * 
		 * Any blocking calls in the Evt API will be safely aborted.
		 */
		bool cancel() noexcept;
		
		/**
		 * Checks whether the subscription has been cancelled.
		 * 
		 * @return true if cancel() has been successfully invoked.
		 */
		bool is_cancelled() const noexcept { return _cancelled; }

		/**
		 * Diagnostic statistics tracking the lifetime of the subscription.
		 */
		struct stats {
			// Total times drain() was invoked.
			size_t drain_count = 0;

			// Total batches requested.
			size_t fetch_count = 0;
			
			// Total batch fetch failures.
			size_t fetch_failed_count = 0;
			
			// Total individual events processed.
			size_t event_count = 0;

			// Total times an event failed to be processed by the handler.
			size_t event_failed_count = 0;

            // Total times an event was rejected (filtered)
            size_t event_filtered_count = 0;
		};

		/**
		 * Returns the current diagnostic statistics for this subscription.
		 * 
		 * @return A copy of the stats structure.
		 */
		const stats& get_stats() const noexcept { return _stats; }

		/**
		 * Returns the underlying signal used to wake up the subscription.
		 * 
		 * @return A reference to the signal instance.
		 */
		const utl::Signal& get_signal() const noexcept { return _signal; }

	private:
		// A signal raised when new events are available.
		utl::Signal _signal;

		// The underlying EVT_HANDLE for the subscription.
		EventSubscriptionHandle _subscription;

		// 
		const EventProcessor::Context _processor_context;

		// Indicates whether the subscription has been drained of all available events.
		bool _drained = false;

		// Indicates whether the subscription has been cancelled.
		bool _cancelled = false;

		// Diagnostic statistics for the subscription.
		stats _stats;
	};

	using EventSubscriptionPtr = std::unique_ptr<EventSubscription>;

}