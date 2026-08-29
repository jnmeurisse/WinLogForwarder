#include "event_queue.h"

#include <cassert>
#include <stdexcept>
#include <mutex>
#include <utility>


namespace wlf::evt {

	EventQueue::EventQueue(size_t capacity)
		: _mutex()
		, _stop_signal(true)
		, _capacity(capacity)
		, _buffer(new EventMessagePtr[capacity])
		, _count(0)
		, _head(0)
		, _tail(0)
	{
        if (_capacity == 0)
            throw std::invalid_argument("EventQueue: invalid capacity");
	}


	bool EventQueue::push(EventMessagePtr message)
	{
        assert(message);
		std::unique_lock<std::mutex> lock(_mutex);

		// Wait until there is space OR the queue has been stopped.
		_cv_space_available.wait(lock, [this]() {
			return _count < _capacity || stopped();
		});

		if (stopped())
			return false;

		_buffer[_tail] = std::move(message);
		_tail = (_tail + 1) % _capacity;
		++_count;

		_cv_event_available.notify_one();
		
		return true;
	}


	EventMessagePtr EventQueue::pop()
	{
		std::unique_lock<std::mutex> lock(_mutex);

		// Wait until there is a message OR the queue has been stopped.
		_cv_event_available.wait(lock, [this]() {
			return _count > 0 || stopped();
		});

		if (stopped())
			return nullptr;

		EventMessagePtr msg = std::move(_buffer[_head]);
		_head = (_head + 1) % _capacity;
		--_count;
		_cv_space_available.notify_one();
		
		return std::move(msg);
	}


	void EventQueue::stop()
	{
		std::lock_guard<std::mutex> lock(_mutex);
        _stop_signal.set();

        // wake up producers and consumer. 
        _cv_space_available.notify_all();
		_cv_event_available.notify_all();
	}


    bool EventQueue::stopped() const
    {
        return _stop_signal.is_set();
    }

}