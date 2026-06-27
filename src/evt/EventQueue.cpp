#include "EventQueue.h"

#include <cassert>
#include <stdexcept>

#include "evt/EventRenderer.h"


namespace evt {

	EventQueue::EventQueue(size_t capacity, size_t message_size)
		: _mutex()
		, _event_available(false)
		, _space_available(false)
		, _capacity(capacity)
		, _slot_payload_bytes(message_size)
		, _buffer(_capacity * (_slot_payload_bytes + sizeof(uint16_t)))
		, _count(0)
		, _head(0)
		, _tail(0)
	{
	}


	bool EventQueue::push(const EventRecord& event_record)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		// 1. Try to reserve a memory slot in the queue
		SlotReservation reservation(*this);
		if (!reservation.get())
			throw std::logic_error("EventQueue::push: queue is full");

		
		// 2. Render the event record in an event message.  
		//    If the renderingfails, the memory slot is released.
		EventMessage message(reservation.get());
		if (!event_record.render(message)) {
			_space_available.set();
			return false;
		}

		// 3. Commit slot changes
		reservation.commit();
		
		// 4. Update both signals immediately matching the new internal counts
		_event_available.set();
		if (_count == _capacity)
			_space_available.reset();

		return true;
	}


	void EventQueue::pop()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_count == 0)
			throw std::logic_error("EventQueue::pop: queue is empty");

		// 1. Adjust internal indices
		release_head();

		// 2. Update both signals immediately matching the new internal counts
		_space_available.set();
		if (_count == 0)
			_event_available.reset();
	}


	EventMessage EventQueue::front()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_count == 0)
			throw std::logic_error("EventQueue::front: queue is empty");

		return EventMessage{ get_slot(_head) };
	}

	
	size_t EventQueue::size() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _count;
	}


	bool EventQueue::empty() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _count == 0;
	}


	bool EventQueue::full() const
	{
		std::lock_guard<std::mutex> lock(_mutex);
		return _count == _capacity;
	}


	uint8_t* EventQueue::get_slot(size_t slot_index) noexcept
	{
		assert(slot_index < _capacity && "invalid slot");
		const size_t stride = _slot_payload_bytes + sizeof(uint16_t);
		return &_buffer[slot_index * stride];
	}


	uint8_t* EventQueue::reserve_slot() noexcept
	{
		if (_count == _capacity)
			return nullptr;

		// get a pointer to the next available slot
		uint8_t* slot = get_slot(_tail);

		// move tail to the next available block
		_tail++;
		_count++;
		if (_tail == _capacity)
			_tail = 0;
			
		return slot;
	}


	void EventQueue::rollback_tail() noexcept
	{
		// handle wrap-around
		_tail = (_tail == 0) 
			? _capacity - 1 
			: _tail - 1;
		_count--;
	}


	void EventQueue::release_head() noexcept
	{
		_head++;
		_count--;
		if (_head == _capacity)
			_head = 0;
	}

}