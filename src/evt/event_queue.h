#pragma once

#include <memory>
#include <mutex>

#include "evt/event_message.h"
#include "utl/signal.h"


namespace wlf::evt {

	/**
	 * @class EventQueue
	 *
	 * A thread-safe, bounded circular ring-buffer for rendering and storing
	 * Windows Event Log Messages, supporting multiple concurrent producers
	 * and a single consumer.
	 *
	 */
	class EventQueue {
	public:
		/**
		 * Constructs the event queue with a fixed number messages.
		 *
		 * @param capacity Maximum number of events the queue can hold.
		 */
		explicit EventQueue(size_t capacity);

		// Prevent copying and moving
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
		 */
		[[nodiscard]]
		bool push(EventMessagePtr message) noexcept;

		/**
		 * Removes the oldest item from the head of the queue.
         * 
         * @return a null pointer if the queue is stopped.
		 */
		[[nodiscard]]
		EventMessagePtr pop() noexcept;

		/** 
		 * Signals that the queue is shutting down.
		 */
		void stop() noexcept;

		// -------------------------------------------------------------------------
		// Capacity queries
		// -------------------------------------------------------------------------

		/**
		 * Returns true if at least one slot is currently available.
		 */
		bool can_push() const noexcept;

		/**
		 * Returns true if a message can be popped immediately by the consumer.
		 */
		bool can_pop() const noexcept;

		// -------------------------------------------------------------------------
		// Stopped signals and state
		// -------------------------------------------------------------------------
		inline utl::Signal& stop_signal() noexcept { return _stop_signal; }
		inline bool stopped() const { return _stop_signal.is_set(); }

	private:
		// Guards all access to this queue's mutable state.
		mutable std::mutex _mutex;

		// Synchronization primitives
		utl::Signal _stop_signal;
		std::condition_variable _cv_space_available;
		std::condition_variable _cv_event_available;

		// Contiguous ring buffer of pointers to EventMessage.
        const size_t _capacity;
        std::unique_ptr < EventMessagePtr[]> _buffer;

		// Number of slots currently occupied.
		size_t _count;

		// Index of the oldest outstanding slot.
		size_t _head;

		// Index one past the most recently reserved slot.
		size_t _tail; 
	};

}