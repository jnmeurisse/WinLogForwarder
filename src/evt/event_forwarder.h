/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*
*/
#pragma once

#include <span>
#include <array>
#include <limits>

#include "evt/event_message.h"
#include "evt/event_queue.h"
#include "evt/event_types.h"
#include "evt/event_thread.h"
#include <net/endpoint.h>
#include "net/tlssocket.h"


namespace wlf::evt {

    class ForwardContext {
    public:
        ForwardContext() = default;

        explicit inline operator bool() const noexcept { return _message.get() != nullptr; }

        bool start(EventMessagePtr message) noexcept;
        void restart();
        bool send(net::TcpSocket& socket);

    private:
        EventMessagePtr _message = nullptr;
        enum class state { sending_length, sending_fragment, completed } _state{ state::sending_length };

        std::array<char, std::numeric_limits<size_t>::digits10> _length = {};
        EventMessage::FragmentList::const_iterator _current_fragment;
        const unsigned char* _buffer = nullptr;
        size_t _size = 0;

    };


	class EventForwarder : public EventThread {
	public:
		EventForwarder(EventQueue& queue, const EventForwarderConfig& config);
        void stop() override;

	protected:
		unsigned int run() override;

	private:
        const EventForwarderConfig _config;

        bool connect(net::TlsSocket& socket, const net::Endpoint& collector);

    };

}