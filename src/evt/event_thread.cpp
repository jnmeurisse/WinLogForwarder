/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#include "event_thread.h"
#include <utility>


namespace wlf::evt {

    EventThread::EventThread(EventQueue& queue)
        : _logger(utl::Logger::instance())
        , _queue(queue)
    {
    }


    void EventThread::stop()
    {
        return _queue.stop();
    }


    bool EventThread::interrupted() const
    {
        return _queue.stopped();
    }


    utl::Signal& EventThread::stop_signal() const noexcept
    {
        return _queue.stop_signal();
    }


    bool EventThread::sleep(unsigned int millisec) const
    {
        return !_queue.stop_signal().wait(millisec);
    }


    bool EventThread::push_message(EventMessagePtr message)
    {
        return _queue.push(std::move(message));
    }


    EventMessagePtr EventThread::pop_message()
    {
        return _queue.pop();
    }

}