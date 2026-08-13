
#pragma once

#include <cstdint>
#include "utl/timer.h"
#include "net/endpoint.h"
#include "net/socket.h"


namespace wlf::net {

	class TcpSocket : public Socket {
	public:
		/**
		 * Constructs a TcpSocket.
		*/
		explicit TcpSocket() noexcept;

		/**
		 * Destroys a TcpSocket object.
		*/
		~TcpSocket() override;

		/**
		 * Connects this socket to the specified end point.
		 *
		 * Note: The MbedTLS network context has no option to specify a timeout.
		 * 	     https://github.com/Mbed-TLS/mbedtls/issues/8027
		 *
		*/
		virtual mbed_err connect(const net::Endpoint& ep, const utl::Timer& timer);

		/**
		 * Reads a sequence of bytes from the socket.
		 *
		 * The function reads data and stores it in the buffer pointed to by the `buf` parameter.
		 * Reading continues until either the buffer is completely filled (as specified by the
		 * `len` parameter) or the specified `timer` elapses, whichever occurs first.
		 *
		 * @param buf Pointer to the buffer where received data will be stored.
		 * @param len The number of bytes to read (i.e., the size of the buffer).
		 * @param timer The maximum amount of time to wait for the operation to complete.
		 *
		 * @return A value of type `rcv_status` indicating the status of the
		 *         read operation.
		 */
		virtual rcv_status read(unsigned char* buf, size_t len, const utl::Timer& timer) noexcept;

		/**
		 * Writes a sequence of bytes to the socket.
		 *
		 * The function sends data from the buffer pointed to by the `buf` parameter to the socket.
		 * Writing continues until all bytes specified by the `len` parameter have been sent, or
		 * the specified `timer` elapses, whichever occurs first.
		 *
		 * @param buf Pointer to the buffer containing the data to be sent.
		 * @param len The number of bytes to send (i.e., the size of the data).
		 * @param timer The maximum amount of time to attempt the write operation.
		 *
		 * @return A value of type `snd_status` indicating the status of the
		 *         write operation.
		 */
		virtual snd_status write(const unsigned char* buf, size_t len, const utl::Timer& timer) noexcept;
	};

}
