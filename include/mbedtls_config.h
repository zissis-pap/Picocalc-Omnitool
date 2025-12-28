#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// Minimal mbedTLS configuration for Telegram HTTPS client
// Based on Pico SDK requirements

// Enable TLS/SSL support
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C

// Cipher suites required for modern HTTPS
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_SSL_PROTO_TLS1_2

// Required crypto primitives
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_MD_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

// Elliptic Curve Cryptography (required for ECDHE)
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED

// Entropy and RNG - Pico SDK provides hardware entropy
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_NO_PLATFORM_ENTROPY  // Embedded system, use custom entropy

// Memory and platform
#define MBEDTLS_PLATFORM_C

// Use standard C library functions
#include <stdio.h>
#include <stdlib.h>

#endif /* MBEDTLS_CONFIG_H */
