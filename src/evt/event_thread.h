/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#pragma once

#include "evt/event_queue.h"
#include "evt/event_message.h"
#include "utl/logger.h"
#include "utl/thread.h"
#include "utl/signal.h"


namespace wlf::evt {

    /**
     * @class EventThread
     * 
     */
    class EventThread : public utl::Thread {
    public:
        EventThread(EventQueue& queue);

        /**
         * Sets the stop signal to signaled.
         * 
         * @throw os_error.
         */
        virtual void stop();

        /**
         * @return true if the stop signal was signaled
         * 
         * @throw os_error.
         */
        bool interrupted() const;

        /**
         * @return a reference to the queue stop signal.
        */
        utl::Signal& stop_signal() const noexcept;

        /**
         * Sleeps until the specified duration is elapsed or this thread is interrupted. 
         * @param milli_sec Duration
         * @return false if the sleep was interrupted.
         * 
         * @throw os_error.
         */
        bool sleep(unsigned int milli_sec) const;

    protected:
        // The application logger
        utl::Logger& _logger;

        // Helper to push and pop messages (could thrown os_error)
        bool push_message(EventMessagePtr message);
        EventMessagePtr pop_message();

    private:
        EventQueue& _queue;
    };

}