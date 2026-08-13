#include "event_forwarder.h"

#include <WinSock2.h>

#include "net/tlsconfig.h"
#include "net/tlssocket.h"
#include "utl/timer.h"
#include <mbedtls/ssl.h>


namespace wlf::evt {

	EventForwarder::EventForwarder(EventQueue& queue)
		: EventThread(queue)
	{
	}


	unsigned int EventForwarder::run()
	{
        enum class State {
            Connecting,
            Connected,
            Disconnected,
            Sending,
            Stopping,
            Stopped
        };


        net::Endpoint host;
        net::TlsConfig config;
        net::TlsSocket socket(config);

        State state = State::Connecting;
        utl::Timer timer;

        while (state != State::Stopped) {
            switch (state) {
            case State::Connecting:
                timer.start(10000);
                switch (socket.connect(host, timer)) {
                case MBEDTLS_ERR_NET_SOCKET_FAILED:
                case MBEDTLS_ERR_SSL_ALLOC_FAILED:
                    state = State::Stopped;
                    break;

                case MBEDTLS_ERR_NET_CONNECT_FAILED:
                case MBEDTLS_ERR_NET_UNKNOWN_HOST:
                default:
                    state = State::Disconnected;
                    break;
                }
                break;

            case State::Connected:
                if (_current_message)
                    state = State::Sending;
                else {
                    _current_message = pop_message();
                    if (!_current_message)
                        state = State::Stopping;
                }
                break;

            case State::Disconnected:
                socket.close();
                //TODO: wait
                break;

            case State::Sending:
                timer.start(30000);
                if (send_message(socket, *_current_message.get(), timer))
                    _current_message.release();
                else
                    state = State::Disconnected;
                break;

            case State::Stopping:
                if (socket.is_connected())
                    socket.shutdown();
                state = State::Stopped;
            }
        }

        return 0;
    }


    bool evt::EventForwarder::send_fragment(net::TcpSocket socket, std::span<const char8_t> buffer, utl::Timer& timer) noexcept
    {
        auto status = socket.write((const unsigned char*)buffer.data(), buffer.size(), timer);
        return status.code == net::snd_status_code::NETCTX_SND_OK;
    }


    bool evt::EventForwarder::send_message(net::TcpSocket socket, evt::EventMessage& message, utl::Timer& timer)
    {
        // Send message length

        // Send message body
        for (auto& fragment : message.fragments())
            if (!send_fragment(socket, fragment.readable_chars(), timer))
                return false;

        return true;
    }

}