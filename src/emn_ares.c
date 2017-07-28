#include "emn.h"
#include "emn_ares.h"
#include "emn_utils.h"

struct emn_ares {
	ev_io ior;
	ev_timer timer;
	struct dns_ctx *ctx;
	struct ev_loop *loop;
	emn_ares_cb_t cb;
};

static void dnscb(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data)
{
	struct emn_ares_query *eq = (struct emn_ares_query *)data;
	eq->ares->cb(eq, result);
}

static void ares_timeout_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	
}

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	struct emn_ares *ares = (struct emn_ares *)w->data;
	dns_ioevent(ares->ctx, time(NULL));
}

struct emn_ares *emn_ares_init(struct ev_loop *loop, emn_ares_cb_t cb)
{
	struct dns_ctx *ctx = NULL;
	struct emn_ares *ares = NULL;

	dns_init(NULL, 0);

	ctx = dns_new(NULL);
	if (!ctx)
		return NULL;

	ares = calloc(1, sizeof(struct emn_ares));
	if (!ares) {
		emn_log(LOG_ERR, "No mem");
		goto err;
	}

	ares->loop = loop;
	ares->ctx = ctx;
	ares->cb = cb;
	
	ev_io_init(&ares->ior, ev_read_cb, dns_open(ctx), EV_READ);
	ares->ior.data = ares;
	ev_io_start(loop, &ares->ior);

	ev_timer_init(&ares->timer, ares_timeout_cb, 1, 1);
	ev_timer_start(loop, &ares->timer);
	return ares;
err:
	emn_ares_free(ares);
	return NULL;
}

void emn_ares_free(struct emn_ares *ares)
{
	if (!ares)
		return;
	
	if (ares->ctx)
		dns_free(ares->ctx);

	ev_io_stop(ares->loop, &ares->ior);
	ev_timer_stop(ares->loop, &ares->timer);

	free(ares);	
}

struct emn_ares_query *emn_ares_resolve(struct emn_ares *ares, const char *name, void *data)
{
	struct dns_query *q = NULL;
	struct emn_ares_query *eq = NULL;

	eq = calloc(1, sizeof(struct emn_ares_query));
	if (!eq)
		return NULL;
	
	q = dns_submit_a4(ares->ctx, name, 0, dnscb, eq); 
	if (!q) {
		emn_log(LOG_ERR, "unable to submit query:%s", dns_strerror(dns_status(ares->ctx)));
		free(eq);
		return NULL;
	}

	eq->ares = ares;
	eq->name = name;
	eq->data = data;
	eq->q = q;

	dns_timeouts(ares->ctx, -1, time(NULL));
	return eq;
}

void emn_ares_query_free(struct emn_ares_query *eq)
{
	if (eq) {
		if (eq->q)
			dns_cancel(eq->ares->ctx, eq->q);
		free(eq);
	}
}
