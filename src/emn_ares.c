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
	ares_destroy(ea->channel);
	free(ea);
}

void dns_callback (void *data, int status, int timeouts, struct hostent *host) 
{	
	struct emn_ares *ea = (struct emn_ares *)data;
	
	if (status != ARES_SUCCESS) {
		fprintf(stderr, "%s\n", ares_strerror(status));
		return;
	}

	ea->cb(host, ea->data);
}

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	ares_channel channel = (ares_channel)w->data;
	ares_process_fd(channel, w->fd, ARES_SOCKET_BAD);
}

int sock_create_callback(ares_socket_t socket_fd, int type, void *userdata)
{
	struct emn_ares *ea = (struct emn_ares *)userdata;
	ares_channel channel = ea->channel;
	
	ev_io_init(&ea->ior, ev_read_cb, socket_fd, EV_READ);
	ea->ior.data = channel;
	ev_io_start(ea->loop, &ea->ior);
	
	return ARES_SUCCESS;
}

static void sock_state_cb(void *data, ares_socket_t socket_fd, int readable, int writable)
{
	struct emn_ares *ea = (struct emn_ares *)data;
	if (!readable && !writable)
		emn_ares_destroy(ea);
}

int emn_resolve(struct ev_loop *loop, const char *name[], emn_resolve_handler_t cb, void *data)
{
	int i;
	struct emn_ares *ea = NULL;
	struct ares_options options = {
		.sock_state_cb = sock_state_cb
	};

	ea = calloc(sizeof(struct emn_ares), 1);
	if (!ea)
		return -1;

	options.sock_state_cb_data = ea;
	ea->loop = loop;
	ea->cb = cb;
	ea->data = data;
	
	if(ares_init_options(&ea->channel, &options, ARES_OPT_SOCK_STATE_CB) != ARES_SUCCESS) {
		free(ea);
		return -1;
	}
	
	ares_set_socket_callback(ea->channel, sock_create_callback, ea);

	for (i = 0; name[i]; i++) {
		/* Initiate a host query by name */
		ares_gethostbyname(ea->channel, name[i], AF_INET, dns_callback, ea);
	}
	
	return 0;
}

