#pragma once

#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>


namespace wlf::net {
    using mbed_err = int;

	class TlsConfig {
	public:
		explicit TlsConfig();
		TlsConfig(const TlsConfig& config) = delete;
		~TlsConfig();

		/**
		 * Defines the CA certificates.
		*/
		void set_ca_crt(mbedtls_x509_crt& ca_crt) noexcept;

		/**
		 * Defines the client certificate.
		*/
		mbed_err set_user_crt(mbedtls_x509_crt& own_crt, mbedtls_pk_context& own_key) noexcept;

		/**
		 * @return the mbedtls_ssl_config.
		*/
		const mbedtls_ssl_config* get_cfg() const noexcept;

	private:
		// All data required to initialize a TLS socket.
		mbedtls_entropy_context _entropy_ctx;
		mbedtls_ctr_drbg_context _ctr_drbg;
		mbedtls_ssl_config _ssl_config;
	};
}