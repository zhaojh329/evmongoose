#include "emn.h"
#include "emn_ares.h"
#include "emn_utils.h"

struct emn_ares {
	ev_io ior;
	ev_timer timer;
	void *data;
	emn_resolve_handler_t cb;
	ares_channel channel;
	struct ev_loop *loop;
};

static void emn_ares_destroy(struct emn_ares *ea)
{
	if (!ea)
		return;
	
	ev_io_stop(ea->loop, &ea->ior);
	ev_timer_stop(ea->loop, &ea->timer);
	ares_destroy(ea->channel);
	free(ea);
}

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

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	fd_set read_fds, write_fds;
	struct emn_ares *ea = (struct emn_ares *)w->data;
	ares_channel channel = ea->channel;

	ares_process_fd(channel, w->fd, ARES_SOCKET_BAD);

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	
	if (ares_fds(channel, &read_fds, &write_fds) == 0)
		emn_ares_destroy(ea);
}

static void ev_timeout_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	struct emn_ares *ea = (struct emn_ares *)w->data;
	
	ea->cb(EMN_ARES_TIMEOUT, NULL, ea->data);
	emn_ares_destroy(ea);
}

int sock_create_callback(ares_socket_t socket_fd, int type, void *userdata)
{
	struct emn_ares *ea = (struct emn_ares *)userdata;
	
	ev_io_init(&ea->ior, ev_read_cb, socket_fd, EV_READ);
	ea->ior.data = ea;
	ev_io_start(ea->loop, &ea->ior);
	
	return ARES_SUCCESS;
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
	
	if(ares_init(&ea->channel) != ARES_SUCCESS) {
		free(ea);
		return -1;
	}
	
	ares_set_socket_callback(ea->channel, sock_create_callback, ea);

	ev_timer_init(&ea->timer, ev_timeout_cb, 2.0, 0);
	ea->timer.data = ea;
	ev_timer_start(loop, &ea->timer);
	
	for (i = 0; name[i]; i++) {
		ares_gethostbyname(ea->channel, name[i], AF_INET, resolve_callback, ea);
	}
	
	return 0;
}

