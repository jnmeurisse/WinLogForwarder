#pragma once

#include "evt/event_queue.h"
#include "evt/event_message.h"
#include "utl/thread.h"
#include "utl/signal.h"

namespace wlf::evt {

    class EventThread : public utl::Thread {
    public:
        EventThread(EventQueue& queue);

        virtual void stop() noexcept;
        bool is_stopped() const;
        utl::Signal& stop_signal() const noexcept;


    protected:
        bool push_message(EventMessagePtr message) noexcept;
        EventMessagePtr pop_message() noexcept;

    private:
        EventQueue& _queue;
    };
}