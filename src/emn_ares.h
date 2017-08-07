#ifndef __EMN_ARES_H_
#define __EMN_ARES_H_

#include <netdb.h>
#include <ares.h>

#define EMN_ARES_SUCCESS	0
#define EMN_ARES_TIMEOUT	1

typedef void (*emn_resolve_handler_t)(int status, struct hostent *host, void *data);

int emn_resolve(struct ev_loop *loop, const char *name[], emn_resolve_handler_t cb, void *data);

#endif
