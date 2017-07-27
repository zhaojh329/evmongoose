#include "emn.h"
#include "emn_ares.h"
#include "emn_utils.h"

static int EMN_ARES_INITED;
static ev_io dns_ior;
static struct ev_loop *dns_loop;

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	dns_ioevent(NULL, time(NULL));
}

int emn_ares(struct ev_loop *loop, const char *domain, dns_query_a4_fn *dnscb)
{
	if (!EMN_ARES_INITED) {
		int fd = dns_init(NULL, 1);
		if (fd < 0) {
			emn_log(LOG_ERR, "unable to initialize dns context");
			return -1;
		}

		EMN_ARES_INITED = 1;
		dns_loop = loop;
		ev_io_init(&dns_ior, ev_read_cb, fd, EV_READ);
		ev_io_start(loop, &dns_ior);
	}	

	if (dns_submit_a4(0, domain, 0, dnscb, (void *)domain) == 0) {
		emn_log(LOG_ERR, "%s: unable to submit query", dns_strerror(dns_status(NULL)));
		goto err;
	}

	dns_timeouts(NULL, -1, time(NULL));
	
	return 0;

err:
	emn_ares_free();
	return -1;
}

void emn_ares_free()
{
	if (EMN_ARES_INITED) {
		EMN_ARES_INITED = 0;
		ev_io_stop(dns_loop, &dns_ior);
		dns_close(NULL);
	}
}

