#include "emn.h"
#include "emn_ares.h"
#include "emn_utils.h"

struct emn_ares {
	void *data;
	emn_resolve_handler_t cb;
	struct ev_loop *loop;
};

struct ares {
	int initialized;
	ares_channel channel;
	struct ares_options options; 
	ev_io iow;		/* IO watcher */
	ev_timer tw;	/* Timer watcher */
};

static struct ares A;

static void resolve_callback (void *data, int status, int timeouts, struct hostent *host) 
{
	struct emn_ares *ea = (struct emn_ares *)data;

	if (status == ARES_EDESTRUCTION)
		return;

	if (status != ARES_SUCCESS) {
		emn_log(LOG_ERR, "ares:%s", ares_strerror(status));
		return;
	}

	ea->cb(EMN_ARES_SUCCESS, host, ea->data);
}

static void ev_io_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	ares_socket_t rfd = ARES_SOCKET_BAD;
	ares_socket_t wfd = ARES_SOCKET_BAD;

    if (revents & EV_READ)
        rfd = w->fd;
	
    if (revents & EV_WRITE)
        wfd = w->fd;

    ares_process_fd(A.channel, rfd, wfd);
}

static void ev_timeout_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	struct emn_ares *ea = (struct emn_ares *)w->data;
	
	ea->cb(EMN_ARES_TIMEOUT, NULL, ea->data);
	
	ares_cancel(A.channel);
}

void sock_state_callback(void *data, int s, int read, int write)
{
	struct emn_ares *ea = (struct emn_ares *)data;
	
	ev_io_stop(ea->loop, &A.iow);
    ev_timer_stop(ea->loop, &A.tw);

	if (read || write) {
        ev_io_set(&A.iow, s, (read ? EV_READ : 0) | (write ? EV_WRITE : 0));
        ev_timer_set(&A.tw, 2, 0);

        ev_io_start(ea->loop, &A.iow);
        ev_timer_start(ea->loop, &A.tw);
    }
}

int emn_resolve(struct ev_loop *loop, const char *name[], emn_resolve_handler_t cb, void *data)
{
	int i;
	struct emn_ares *ea = NULL;

	ea = calloc(sizeof(struct emn_ares), 1);
	if (!ea)
		return -1;

	ea->loop = loop;
	ea->cb = cb;
	ea->data = data;

	if (!A.initialized) {
		A.options.sock_state_cb_data = ea;
	    A.options.sock_state_cb = sock_state_callback;
	    A.options.flags = ARES_FLAG_NOCHECKRESP;

		ev_init(&A.iow, ev_io_cb);
		A.iow.data = ea;
	    
		ev_timer_init(&A.tw, ev_timeout_cb, 2, 0);
		A.tw.data = ea;
		
		if(ares_init_options(&A.channel, &A.options, ARES_OPT_SOCK_STATE_CB | ARES_OPT_FLAGS) != ARES_SUCCESS) {
			free(ea);
			return -1;
		}

		A.initialized = 1;
	}
	
	for (i = 0; name[i]; i++) {
		ares_gethostbyname(A.channel, name[i], AF_INET, resolve_callback, ea);
	}
	
	return 0;
}

int emn_resolve_single(struct ev_loop *loop, const char *name, emn_resolve_handler_t cb, void *data)
{
	const char *names[] = {name, NULL};
	return emn_resolve(loop, names, cb, data);
}

