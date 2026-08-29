#include "tlssocket.h"


namespace wlf::net {


	TlsSocket::TlsSocket(const TlsConfig& tls_config) :
		TcpSocket(),
		_tlscfg{ tls_config },
		_enable_hostname_verification{ false }
	{
	}


	TlsSocket::~TlsSocket()
	{
	}


	void TlsSocket::set_hostname_verification(bool enable_verification) noexcept
	{
		_enable_hostname_verification = enable_verification;
	}


	mbed_err TlsSocket::connect(const Endpoint& ep, const utl::Timer& timer)
	{
		mbed_err rc = TcpSocket::connect(ep, timer);
        if (rc) return rc;

		rc = _tlsctx.configure(*_tlscfg.get_cfg(), *netctx());
        if (rc) return rc;

        if (_enable_hostname_verification) {
            rc = _tlsctx.set_hostname(ep.hostname());
        }
        else {
            rc = _tlsctx.set_hostname("");
        }

		return rc;
	}


    void net::TlsSocket::close() noexcept
    {
        _tlsctx.clear();
        TcpSocket::close();
    }


	tls_handshake_status TlsSocket::handshake(const utl::Timer& timer) noexcept
	{
		tls_handshake_status handshake_status;
		do {
			handshake_status = _tlsctx.handshake();

			if (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_IO) {
				const poll_status poll_status = poll(handshake_status.rc, timer.remaining_time());

				if (poll_status.code != poll_status_code::NETCTX_POLL_OK) {
					handshake_status.status_code = hdk_status_code::SSLCTX_HDK_ERROR;
					handshake_status.rc = poll_status.rc;
				}
				else
					// Noop, poll succeeded
					;
			}
			else if (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_ASYNC) {
				if (timer.is_elapsed()) {
					handshake_status.status_code = hdk_status_code::SSLCTX_HDK_ERROR;
					handshake_status.rc = MBEDTLS_ERR_SSL_TIMEOUT;
				}
				else {
                    // Sleep 100 ms
                    ::mbedtls_net_usleep(100000);
				}
			}
		} while (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_IO ||
			handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_ASYNC);

		return handshake_status;
	}


	mbed_err TlsSocket::shutdown() noexcept
    {
		mbed_err rc = 0;

		if (is_connected()) {
			// Notify the peer that the connection is being closed.
			utl::Timer timer(5000);
			bool keep_closing = true;

			while (keep_closing) {
				const net::tls_close_status close_status = _tlsctx.close_notify();
				if (close_status.status_code == close_status_code::SSLCTX_CLOSE_RETRY) {
					// Handle the "Busy/Retry" case
					const poll_status poll_wait_status = poll(close_status.rc, timer.remaining_time());

					if (poll_wait_status.code != poll_status_code::NETCTX_POLL_OK) {
						keep_closing = false;
						rc = poll_wait_status.rc;
					}
				}
				else {
					keep_closing = false;
					rc = close_status.rc;
				}
			}

			// shutdown and close the socket even if the close notify failed.
			TcpSocket::shutdown();
		}

		_tlsctx.clear();

		return rc;
	}


	mbed_err TlsSocket::get_crt_check() const noexcept
	{
		return _tlsctx.get_crt_check();
	}


	const char* TlsSocket::get_ciphersuite() const noexcept
	{
		return _tlsctx.get_ciphersuite();
	}


	const char* TlsSocket::get_tls_version() const noexcept
	{
		return _tlsctx.get_tls_version();
	}


	const mbedtls_x509_crt* TlsSocket::get_peer_crt() const noexcept
	{
		return _tlsctx.get_peer_crt();
	}


	const TlsConfig& TlsSocket::get_tls_config() const noexcept
	{
		return _tlscfg;
	}


	net::rcv_status TlsSocket::recv_data(unsigned char* buf, const size_t len) noexcept
	{
		return _tlsctx.recv_data(buf, len);
	}


	net::snd_status TlsSocket::send_data(const unsigned char* buf, const size_t len) noexcept
	{
		return _tlsctx.send_data(buf, len);
	}

}
