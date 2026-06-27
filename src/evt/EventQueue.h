#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "evt/EventMessage.h"
#include "evt/EventRecord.h"
#include "utl/Event.h"


namespace evt {
	/**
	 * @class EventQueue
	 * 
	 * A thread-safe, bounded circular ring-buffer for rendering and storing 
	 * Windows Event Log Messages for a single producer and a single consumer.
	 *
	 * This class is not intended for multiple concurrent producers
	 * or multiple concurrent consumers.
	 */
	class EventQueue {
	public:
		/**
		 * Constructs the event queue with a fixed number of slots and message size.
		 * 
		 * @param capacity Maximum number of events the queue can hold.
		 * @param message_size The maximum message size limit per individual event.
		 */
		explicit EventQueue(size_t capacity, size_t message_size);

		/**
		* Destructs this EventQueue.
		*/ 
		~EventQueue() = default;

		// Prevent copying and moving
		EventQueue(const EventQueue&) = delete;
		EventQueue& operator=(const EventQueue&) = delete;
		EventQueue(EventQueue&&) = delete;
		EventQueue& operator=(EventQueue&&) = delete;

		// -------------------------------------------------------------------------
		// Queue operations
		// -------------------------------------------------------------------------

		/**
		 * Renders and pushes an event into the queue.
		 * 
		 * @param event_record The abstract record containing the data to encode.
		 * @return true if rendering succeeded; false if not.
		 * 
		 * @throws std::logic_error if the queue is full.
		 */
		bool push(const EventRecord& event_record);

		/**
		 * Removes the oldest item from the head of the queue.
		 * 
		 * @throws std::logic_error if the queue contains no elements.
		*/
		void pop();

		/**
		 * Returns the oldest queued event without removing it.
		 * @return An EventMessage pointing directly to the active front slot.
		 * 
		 * @note The returned EventMessage remains valid until dequeue() or destruction
		 *       of the queue.
		 * 
		 * @throws std::logic_error if the queue contains no elements.
		*/
		EventMessage front();
		
		// -------------------------------------------------------------------------
		// Capacity queries
		// -------------------------------------------------------------------------

		/** 
		 * Returns the number of events currently in the queue. 
		 */
		size_t size() const;

		/** 
		 * Returns true if the queue is empty. 
		 */
		bool empty() const;

		/**
		 *  Returns true if the queue is full.
		 */
		bool full() const;

		// -------------------------------------------------------------------------
		// Capacity signals
		// -------------------------------------------------------------------------
		inline utl::Event& event_available_event() noexcept { return _event_available; }
		inline utl::Event& space_available_event() noexcept { return _space_available; }

	private:
		class SlotReservation {
		public:
			SlotReservation(EventQueue& q) noexcept
				: _q(q)
				, _slot(q.reserve_slot()) 
			{
			}

			~SlotReservation()
			{
				if (_slot)
					_q.rollback_tail();
			}

			SlotReservation(const SlotReservation&) = delete;
			SlotReservation& operator=(const SlotReservation&) = delete;

			void commit() noexcept { _slot = nullptr; }

			uint8_t* get() const noexcept { return _slot; }

		private:
			EventQueue& _q;
			uint8_t* _slot;
		};

	private:
		// Guards all access to this queue.
		mutable std::mutex _mutex;

		// Event set when an event was pushed in the queue.
		utl::Event _event_available;

		// Event set when space was made available.
		utl::Event _space_available;

		// Maximum number of event message stored in the queue
		const size_t _capacity;

		// Size of an event message stored in the queue
		const size_t _slot_payload_bytes;

		// A contiguous ring buffer of memory blocks holding messages
		std::vector<uint8_t> _buffer;

		// Total number of events in the queue.
		size_t _count;

		// The head and tail indices are used to manage the ring buffer of slots.
		// Align allocation to prevent false sharing across CPU cache lines.
		alignas(64) size_t _head;		// Index of oldest slot.
		alignas(64) size_t _tail;		// Index one past newest slot.

		uint8_t* get_slot(size_t slot_index) noexcept;
		uint8_t* reserve_slot() noexcept;
		void rollback_tail() noexcept;
		void release_head() noexcept;
	};

}
