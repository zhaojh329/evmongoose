#ifndef __EMN_SSL_H_
#define __EMN_SSL_H_

#include "emn_internal.h"

#if (EMN_USE_OPENSSL)

#include <openssl/err.h>

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);
ssize_t emn_ssl_read(SSL *ssl, void *buf, size_t count);
ssize_t emn_ssl_write(SSL *ssl, void *buf, size_t count);

#elif (EMN_USE_CYASSL)

#include <wolfssl/error-ssl.h>

WOLFSSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);
ssize_t emn_ssl_read(WOLFSSL *ssl, void *buf, size_t count);
ssize_t emn_ssl_write(WOLFSSL *ssl, void *buf, size_t count);

#endif

int emn_ssl_accept(struct emn_client *cli);

#endif
