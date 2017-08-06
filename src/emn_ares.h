#ifndef __EMN_ARES_H_
#define __EMN_ARES_H_

#include <ares.h>

int emn_ares_resolve(struct ev_loop *loop, const char *name[], void *data);

#endif
