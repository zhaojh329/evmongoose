#ifndef __EMN_ARES_H_
#define __EMN_ARES_H_

#include <udns.h>

int emn_ares(struct ev_loop *loop, const char *domain, dns_query_a4_fn *dnscb);
void emn_ares_free();

#endif
