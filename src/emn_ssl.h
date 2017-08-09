#ifndef __EMN_SSL_H_
#define __EMN_SSL_H_

#include "emn_internal.h"

#if (EMN_USE_OPENSSL)

#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);

static inline ssize_t emn_ssl_read(SSL *ssl, void *buf, size_t count)
{
	return SSL_read(ssl, buf, count);
}

static inline ssize_t emn_ssl_write(SSL *ssl, void *buf, size_t count)
{
	return SSL_write(ssl, buf, count);
}

#elif (EMN_USE_CYASSL)

WOLFSSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);

static inline ssize_t emn_ssl_read(WOLFSSL *ssl, void *buf, size_t count)
{
	return wolfSSL_read(ssl, buf, count);
}

static inline ssize_t emn_ssl_write(WOLFSSL *ssl, void *buf, size_t count)
{
	return wolfSSL_write(ssl, buf, count);
}

#endif

int emn_ssl_accept(struct emn_client *cli);

#endif
