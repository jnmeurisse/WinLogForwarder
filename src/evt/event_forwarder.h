#pragma once

#include <span>

#include "evt/event_message.h"
#include "evt/event_queue.h"
#include "evt/event_thread.h"
#include "net/tcpsocket.h"
#include "utl/timer.h"

namespace wlf::evt {

	class EventForwarder : public EventThread {
	public:
		EventForwarder(EventQueue& queue);

	protected:
		unsigned int run() override;

	private:
        evt::EventMessagePtr _current_message = nullptr;

        static bool send_fragment(net::TcpSocket socket, std::span<const char8_t> buffer, utl::Timer& timer) noexcept;

        static bool send_message(net::TcpSocket socket, evt::EventMessage& message, utl::Timer& timer);
    };

}