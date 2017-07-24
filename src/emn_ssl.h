#ifndef __EMN_SSL_H_
#define __EMN_SSL_H_

#include "emn_internal.h"

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type);

#endif
