#ifndef __EMN_SSL_H_
#define __EMN_SSL_H_

#include "emn_internal.h"

#if (EMN_USE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);
#elif (EMN_USE_CYASSL)
WOLFSSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);
#endif

#endif
