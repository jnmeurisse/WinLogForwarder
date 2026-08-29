#pragma once

#include <memory>
#include <mutex>

#include "evt/event_message.h"
#include "utl/signal.h"


namespace wlf::evt {

	/**
	 * @class EventQueue
	 *
	 * A thread-safe, bounded circular ring-buffer for storing
	 * Windows Event Log Messages, supporting multiple concurrent 
     * producers and a single consumer.
	 */
	class EventQueue {
	public:
		/**
		 * Constructs an event queue that can hold a fixed number messages.
		 *
		 * @param capacity Maximum number of events the queue can hold.
         * @throw invalid_argument if capacity is null.
         * @throw os_error if stop signal can not be allocated.
		 */
		explicit EventQueue(size_t capacity);

		// Prevent copying and moving the queue
		EventQueue(const EventQueue&) = delete;
		EventQueue& operator=(const EventQueue&) = delete;
		EventQueue(EventQueue&&) = delete;
		EventQueue& operator=(EventQueue&&) = delete;

		// -------------------------------------------------------------------------
		// Queue operations
		// -------------------------------------------------------------------------

		/**
		 * Appends a message to the queue.  The function waits until there is
         * a free space in the queue.
         * 
		 * @param message The message to append to the queue.
		 * @return false if the queue is stopped.
         * 
         * @throw os_error If an OS error occurred when checking the stop signal.
         */
		[[nodiscard]]
		bool push(EventMessagePtr message);

		/**
		 * Removes the oldest item from the head of the queue.  The function waits
         * until there is a message in the queue.
         * 
         * @return a null pointer if the queue is stopped.
         * @throw os_error If an OS error occurred when checking the stop signal.
         */
		[[nodiscard]]
		EventMessagePtr pop();

		/** 
		 * Signals that the queue must stop.  All push and pop will fail
         * once the queue is in stopped state.
         * 
         * @throw os_error If an OS error occurred when setting the stop signal.
		 */
		void stop();

		// -------------------------------------------------------------------------
		// Stopped signals and state
		// -------------------------------------------------------------------------

		/**
		 * @return a reference to the stop signal. 
		 */
		inline utl::Signal& stop_signal() noexcept { return _stop_signal; }

        /**
         * @return True if the stop signal was signaled.
         * 
         * @throw os_error If an OS error occurred when setting the stop signal.
         */
        bool stopped() const;

	private:
		// Guards all access to this queue's mutable state.
		mutable std::mutex _mutex;

		// Synchronization primitives
		utl::Signal _stop_signal;
		std::condition_variable _cv_space_available;
		std::condition_variable _cv_event_available;

		// Contiguous ring buffer of pointers to EventMessage.
        const size_t _capacity;
        std::unique_ptr <EventMessagePtr[]> _buffer;

		// Number of slots currently occupied.
		size_t _count;

		// Index of the oldest outstanding slot.
		size_t _head;

		// Index one past the most recently reserved slot.
		size_t _tail;
	};

}