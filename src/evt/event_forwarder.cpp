#include "event_forwarder.h"

#include <WinSock2.h>
#include <array>
#include <charconv>
#include <exception>
#include <span>
#include "net/tlsconfig.h"
#include "utl/timer.h"

#include <mbedtls/ssl.h>


namespace wlf::evt {

    bool ForwardContext::start(EventMessagePtr message) noexcept
    {
        _message = std::move(message);
        restart();
        return _message != nullptr;
    }


    void ForwardContext::restart()
    {
        if (_message) {
            auto [ptr, ec] = std::to_chars(_length.data(), _length.data() + _length.size(), _message->size());
            if (ec != std::errc()) {
                _state = state::completed;
                _message.release();
                return;
            }

            _state = state::sending_length;
            _buffer = (const unsigned char*)_length.data();
            _size = static_cast<size_t>(ptr - _length.data());
        }
    }


    bool ForwardContext::send(net::TcpSocket& socket)
    {
        if (_size > 0) {
            const net::snd_status status = socket.send_data(_buffer, _size);
            switch (status.code) {
            case net::snd_status_code::NETCTX_SND_ERROR:
                // network error 
                return false;

            case net::snd_status_code::NETCTX_SND_RETRY:
                // retry later
                return true;

            case net::snd_status_code::NETCTX_SND_OK:
                _size -= status.sbytes;
                _buffer += status.sbytes;
                break;
            }
        }

        if (_size == 0) {
            std::span<const char8_t> fragment_block;

            switch (_state) {
            case state::sending_length:
                _state = state::sending_fragment;
                _current_fragment = _message->fragments().begin();
                fragment_block = _current_fragment->readable_chars();
                _size = fragment_block.size();
                _buffer = (const unsigned char*)fragment_block.data();
                break;

            case state::sending_fragment:
                _current_fragment = ++_current_fragment;
                if (_current_fragment == _message->fragments().cend())
                    // End of fragments list
                    _state = state::completed;
                else {
                    // Next fragment in the list
                    fragment_block = _current_fragment->readable_chars();
                    _size = fragment_block.size();
                    _buffer = (const unsigned char*)fragment_block.data();
                }
                break;

            default:
                _message.release();
                _buffer = nullptr;
                _size = 0;
                break;
            }
        }

        return true;
    }


	EventForwarder::EventForwarder(EventQueue& queue, const EventForwarderConfig& config)
		: EventThread(queue)
        , _config(config)
	{
	}


    void EventForwarder::stop()
    {
        _logger.info("Stopping event forwarder...");
        EventThread::stop();
    }


    bool EventForwarder::connect(net::TlsSocket& socket, const net::Endpoint& collector)
    {
        utl::Timer timer;

        switch (socket.connect(collector, timer)) {
        case 0:
            break;

        case MBEDTLS_ERR_SSL_ALLOC_FAILED:
            throw std::bad_alloc();

        default:
            return false;
        }

        timer.start(15000);
        return socket.handshake(timer).status_code == net::hdk_status_code::SSLCTX_HDK_OK;
    }


	unsigned int EventForwarder::run()
	{
        _logger.info("Starting event forwarder...");

        enum class State {
            Connecting,
            Connected,
            Disconnected,
            WaitingMessage,
            Sending,
            Stopping,
            Stopped
        };

        WSAEVENT write_event = ::WSACreateEvent();



        net::TlsConfig config;
        net::TlsSocket socket(config);
        socket.set_hostname_verification(true);

        State state = State::Connecting;
        utl::Timer timer;
        ForwardContext context{};

        while (state != State::Stopped) {
            switch (state) {
            case State::Connecting:
                if (!connect(socket, _config.collector))
                    state = State::Disconnected;
                else
                    state = State::Connected;
                break;

            case State::Connected:
                if (context) {
                    state = State::Sending;
                    context.restart();
                }
                else
                    state = State::WaitingMessage;
                break;

            case State::Disconnected:
                socket.close();
                if (!sleep(10000))
                    state = State::Stopping;
                else
                    state = State::Connecting;
                break;

            case State::WaitingMessage:
                if (context.start(std::move(pop_message())))
                    state = State::Sending;
                else
                    state = State::Stopping;
                break;

            case State::Sending:
                if (context) {
                    ::WSAEventSelect(socket.get_fd(), write_event, FD_WRITE | FD_CLOSE);
                    std::array<::HANDLE, 2> events{
                        stop_signal().handle(),
                        write_event
                    };

                    switch (::WSAWaitForMultipleEvents(2, events.data(), false, INFINITE, false)) {
                    case 0:
                        break;
                    }

                    ::WSANETWORKEVENTS network_events;
                    ::WSAEnumNetworkEvents(socket.get_fd(), write_event, &network_events);
                    if (network_events.lNetworkEvents & FD_CLOSE_BIT)
                        state = State::Disconnected;
                    else if (network_events.lNetworkEvents & FD_WRITE_BIT) {
                        if (!context.send(socket))
                            state = State::Disconnected;
                    }
                }
                else
                    state = State::WaitingMessage;
                break;

            case State::Stopping:
                if (socket.is_connected())
                    socket.shutdown();
                state = State::Stopped;
            }
        }

        return 0;
    }

}