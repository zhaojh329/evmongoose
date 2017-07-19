#include "emn.h"
#include "list.h"
#include "ebuf.h"

struct emn_server {
	int sock;
	struct ev_loop *loop;
	ev_io ior;
	struct list_head list;
};

struct emn_client {
	int sock;
	struct emn_server *srv;
	ev_io ior;
	ev_io iow;
	struct ebuf rbuf;
	struct ebuf sbuf;
	struct list_head node;
};

static void set_non_blocking_mode(int sock)
{
	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

/*
 * Address format: [PROTO://][HOST]:PORT
 *
 * HOST could be IPv4/IPv6 address or a host name.
 * `host` is a destination buffer to hold parsed HOST part.
 * `proto` is a returned socket type, either SOCK_STREAM or SOCK_DGRAM
 *
 * Return:
 *   -1   on parse error
 *    0   if HOST needs DNS lookup
 *   >0   length of the address string
 */
static int parse_address(const char *str, struct sockaddr_in *sin, int *proto, char *host, size_t host_len)
{
	return 0;
}

/* 'sin' must be an initialized address to bind to */
static int open_listening_socket(struct sockaddr_in *sin, int type, int proto)
{
	int sock = -1;
	int on = 1;

	sock = socket(sin->sin_family, type, proto);
	if (sock < 0)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in)) < 0) {
		close(sock);
		return -1;
	}

	if (type == SOCK_STREAM && listen(sock, SOMAXCONN) < 0) {
		close(sock);
		return -1;
	}

	set_non_blocking_mode(sock);
	
	return sock;
}

struct emn_server *emn_bind(const char *address, emn_event_handler_t event_handler)
{
	struct sockaddr_in sin;
	struct emn_server *srv = NULL;
	int proto;
	char host[128];
	
	if (parse_address(address, &sin, &proto, host, sizeof(host)) <= 0) {
		printf("cannot parse address\n");
		return NULL;
	}

	srv = calloc(1, sizeof(struct emn_server));
	if (!srv)
		return NULL;
	
	INIT_LIST_HEAD(&srv->list);
	
	srv->sock = open_listening_socket(&sin, proto, proto);
	if (srv->sock < 0) {
		printf("cannot open_listening_socket\n");
		emn_server_destroy(srv);
		return NULL;
	}
	
	return srv;
}

void emn_server_destroy(struct emn_server *srv)
{
	if (srv) {
		if (srv->sock > 0)
			close(srv->sock);
		free(srv);
	}
}