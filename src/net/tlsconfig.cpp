#include "tlsconfig.h"


namespace wlf::net {

	// Recommended ciphers from https://ciphersuite.info. 
	static const int default_ciphers[] = {
		// TLS 1.3 cipher suites
		MBEDTLS_TLS1_3_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS1_3_AES_128_GCM_SHA256,

		// TLS 1.2 ciphe rsuites
		//    Key exchange   : elliptic curve diffie-hellman key exchange
		//    Authentication : RSA
		//    Encryption     : CHACHA20 or AES
		//    Message auth   : SHA256 
		// recommended
		MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,

		// RFC 6460 : suite B TLS 1.2
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,

		// secure (no perfect Forward Secrecy)
		MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256,

		// mandatory supported by all tls 1.2 server (RFC5246)
		MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,

		// end of list
		0
	};


	TlsConfig::TlsConfig()
	{
		::mbedtls_entropy_init(&_entropy_ctx);

		::mbedtls_ctr_drbg_init(&_ctr_drbg);
		::mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy_ctx, nullptr, 0);

		::mbedtls_ssl_config_init(&_ssl_config);
		::mbedtls_ssl_config_defaults(&_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		::mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
		::mbedtls_ssl_conf_rng(&_ssl_config, mbedtls_ctr_drbg_random, &_ctr_drbg);

		// 1.2 and 1.3 are accepted
		::mbedtls_ssl_conf_min_tls_version(&_ssl_config, MBEDTLS_SSL_VERSION_TLS1_2);

		// set cipher list
		::mbedtls_ssl_conf_ciphersuites(&_ssl_config, default_ciphers);
	}

	TlsConfig::~TlsConfig()
	{
		// free all memory allocated by SSL library
		::mbedtls_ssl_config_free(&_ssl_config);
		::mbedtls_ctr_drbg_free(&_ctr_drbg);
		::mbedtls_entropy_free(&_entropy_ctx);
	}

    
	void TlsConfig::set_ca_crt(mbedtls_x509_crt& ca_crt) noexcept
	{
		::mbedtls_ssl_conf_ca_chain(&_ssl_config, &ca_crt, nullptr);
		::mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);
	}


	mbed_err TlsConfig::set_user_crt(mbedtls_x509_crt& own_crt, mbedtls_pk_context& own_key) noexcept
	{
		return ::mbedtls_ssl_conf_own_cert(&_ssl_config, &own_crt, &own_key);
	}


	const mbedtls_ssl_config* TlsConfig::get_cfg() const noexcept
	{
		return &_ssl_config;
	}

}
