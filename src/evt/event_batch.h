#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winevt.h>

#include <array>
#include "evt/event_log_handle.h"
#include "evt/event_subscription_handle.h"


namespace wlf::evt {

    /**
     * Manages a batch of Windows Event Log events retrieved from a subscription.
     *
     */
    class EventBatch {
    public:
        EventBatch() = delete;

        /**
        * Constructs an EventBatch tied to a specific subscription.
        * 
        * @param subscription A valid handle to an active event subscription. 
        * Note: EventBatch does NOT take ownership of this subscription handle.        
        */
        explicit EventBatch(const EventSubscriptionHandle& subscription) noexcept;

        /**
        * Cleans up any fetched event handles that were not consumed.
        */
        ~EventBatch();

        enum class FetchResult
        {
            Continue,   // Process this batch and fetch again
            Completed,  // Process this batch; nothing more to fetch
			Cancelled,  // The subscription was cancelled
            Failed
        };

        /** 
         * Fetches the next batch of events into the internal buffer.
         * 
         * @param timeout The maximum time (in ms) to block while waiting for events.
         * @return FetchResult indicating if more events are available.
        */
        FetchResult fetch(DWORD timeout) noexcept;

        /**
        * Retrieves the next available event from the current batch.
        * 
        * @return an EventHandle. EventHandle is empty if there is no more 
        * event available in the batch.
        */
        EventLogHandle next() noexcept;

    private:
        // Helper to safely close any unconsumed handles in the current batch.
        void clear_unconsumed_events() noexcept;

        // The maximum number of events to request from the OS in a single fetch call.
        static constexpr size_t _batch_size = 32;

        // The active Windows Event Log subscription handle. 
        // Note: This handle is owned by the caller/system, not by this class.
        const EventSubscriptionHandle& _subscription;

        // The internal buffer holding raw event handles for the current batch.
        std::array<::EVT_HANDLE, _batch_size> _events{nullptr};

        // The total number of valid, unconsumed events currently sitting in the
        // _events buffer.
        ::DWORD _count = 0;

        // The array index of the next event to be yielded by the next() method.
        ::DWORD _next = 0;
    };

}