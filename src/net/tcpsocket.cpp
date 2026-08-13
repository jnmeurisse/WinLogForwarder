#include "net/tcpsocket.h"

namespace wlf::net {

	TcpSocket::TcpSocket() noexcept :
		Socket()
	{
	}


	TcpSocket::~TcpSocket()
	{
	}


	mbed_err TcpSocket::connect(const net::Endpoint& ep, const utl::Timer& timer)
	{
		mbed_err rc = Socket::connect(ep, net_protocol::NETCTX_PROTO_TCP, timer);
		if (rc)
			goto terminate;

		rc = Socket::set_blocking_mode(false);
		if (rc)
			goto terminate;

	terminate:
		return rc;
	}


	rcv_status TcpSocket::read(unsigned char* buf, size_t len, const utl::Timer& timer) noexcept
	{
		rcv_status read_status { rcv_status_code::NETCTX_RCV_OK, 0, 0 };

		bool keep_reading = len > 0;
		while (keep_reading) {
			const rcv_status rcv_data_status =  recv_data(buf, len);

			read_status.code = rcv_data_status.code;
			read_status.rc = rcv_data_status.rc;

			if (read_status.code == rcv_status_code::NETCTX_RCV_OK) {
				// Update progress
				buf += rcv_data_status.rbytes;
				len -= rcv_data_status.rbytes;
				read_status.rbytes += rcv_data_status.rbytes;

				keep_reading = len > 0;
			}
			else if (read_status.code == rcv_status_code::NETCTX_RCV_RETRY) {
				// Handle the "Busy/Retry" case
				const poll_status poll_rcv_status = poll(read_status.rc, timer.remaining_time());

				if (poll_rcv_status.code != poll_status_code::NETCTX_POLL_OK) {
					read_status.code = rcv_status_code::NETCTX_RCV_ERROR;
					read_status.rc = poll_rcv_status.rc;
					keep_reading = false;
				}
			}
			else
				keep_reading = false;
		}

		return read_status;
	}


	snd_status TcpSocket::write(const unsigned char* buf, size_t len, const utl::Timer& timer) noexcept
	{
		snd_status write_status { snd_status_code::NETCTX_SND_OK, 0, 0 };

		bool keep_writing = len > 0;
		while (keep_writing) {
			const snd_status snd_data_status{ send_data(buf, len) };

			write_status.code = snd_data_status.code;
			write_status.rc = snd_data_status.rc;

			if (write_status.code == snd_status_code::NETCTX_SND_OK) {
				// Update progress
				buf += snd_data_status.sbytes;
				len -= snd_data_status.sbytes;
				write_status.sbytes += snd_data_status.sbytes;

				keep_writing = len > 0;
			}
			else if (write_status.code == snd_status_code::NETCTX_SND_RETRY) {
				// Handle the "Busy/Retry" case
				const poll_status poll_snd_status{ poll(write_status.rc, timer.remaining_time()) };

				if (poll_snd_status.code != poll_status_code::NETCTX_POLL_OK) {
					write_status.code = snd_status_code::NETCTX_SND_ERROR;
					write_status.rc = poll_snd_status.rc;
					keep_writing = false;
				}
			}
			else
				keep_writing = false;
		}

		return write_status;
	}

};
