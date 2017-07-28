#ifndef __EMN_ARES_H_
#define __EMN_ARES_H_

#include <udns.h>

struct emn_ares;
struct emn_ares_query;

typedef void (*emn_ares_cb_t)(struct emn_ares_query *eq, struct dns_rr_a4 *result);

struct emn_ares_query {
	struct emn_ares *ares;
	struct dns_query *q;
	const char *name;
	void *data;
};

struct emn_ares *emn_ares_init(struct ev_loop *loop, emn_ares_cb_t cb);
void emn_ares_free(struct emn_ares *ares);

struct emn_ares_query *emn_ares_resolve(struct emn_ares *ares, const char *name, void *data);
void emn_ares_query_free(struct emn_ares_query *eq);


#endif
