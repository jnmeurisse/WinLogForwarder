#include "event_thread.h"

namespace wlf::evt {

    EventThread::EventThread(EventQueue& queue)
        : _queue(queue)
    {
    }


    void EventThread::stop() noexcept
    {
        _queue.stop();
    }


    bool EventThread::is_stopped() const
    {
        return _queue.stopped();
    }


    utl::Signal& EventThread::stop_signal() const noexcept
    {
        return _queue.stop_signal();
    }


    bool EventThread::push_message(EventMessagePtr message) noexcept
    {
        return _queue.push(std::move(message));
    }


    EventMessagePtr EventThread::pop_message() noexcept
    {
        return _queue.pop();
    }

}