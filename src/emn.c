#include "emn.h"
#include "list.h"
#include "ebuf.h"

#define EMN_TYPE_SERVER	1
#define EMN_TYPE_CLIENT	2

struct emn_object {
	int type;
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
};

struct emn_server {
	int type;
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	struct ev_loop *loop;
	ev_io ior;
	struct list_head client_list;
};

struct emn_client {
	int type;
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	struct emn_server *srv;
	ev_io ior;
	ev_io iow;
	struct ebuf rbuf;
	struct ebuf sbuf;
	struct list_head list;
};

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
static int parse_address(const char *str, struct sockaddr_in *sin, int *proto)
{
	unsigned int a, b, c, d, port = 0;
	int len = 0;

	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;

	*proto = SOCK_STREAM;

	if (strncmp(str, "udp://", 6) == 0) {
		str += 6;
		*proto = SOCK_DGRAM;
	} else if (strncmp(str, "tcp://", 6) == 0) {
		str += 6;
	}
	
	if (sscanf(str, "%u.%u.%u.%u:%u%n", &a, &b, &c, &d, &port, &len) == 5) {
		/* Bind to a specific IPv4 address, e.g. 192.168.1.1:8080 */
		sin->sin_addr.s_addr = htonl(((uint32_t)a << 24) | ((uint32_t)b << 16) | c << 8 | d);
		sin->sin_port = htons((uint16_t)port);
	} else if (sscanf(str, "%u%n", &port, &len) == 1) {
		/* If only port is specified, bind to IPv4, INADDR_ANY */
		sin->sin_port = htons((uint16_t)port);
	} else {
		return -1;
	}
	
	return len;
}

static int open_listening_socket(struct sockaddr_in *sin, int type, int proto)
{
	int sock = -1;
	int on = 1;

	sock = socket(sin->sin_family, type | SOCK_NONBLOCK | SOCK_CLOEXEC, proto);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		close(sock);
		return -1;
	}

	if (type == SOCK_STREAM && listen(sock, SOMAXCONN) < 0) {
		perror("listen");
		close(sock);
		return -1;
	}
	
	return sock;
}

static void emn_call(struct emn_object *obj, emn_event_handler_t ev_handler, int event, void *data)
{
	if (ev_handler == NULL) {
		/*
		* If protocol handler is specified, call it. Otherwise, call user-specified
		* event handler.
		*/
		ev_handler = obj->proto_handler ? obj->proto_handler : obj->handler;
	}
	
	if (ev_handler)
		ev_handler(obj, event, data);
}

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	ssize_t len;
	static char *buf;
	struct emn_client *cli = (struct emn_client *)w->data;
	
	if (!buf) {
		buf = malloc(EMN_TCP_RECV_BUFFER_SIZE);
		if (!buf) {
			perror("malloc");
			return;
		}
	}
	
	len = read(w->fd, buf, EMN_TCP_RECV_BUFFER_SIZE);
	if (len > 0) {
		ebuf_append(&cli->rbuf, buf, len);
		printf("%zu %.*s\n", cli->rbuf.len, (int)cli->rbuf.len, cli->rbuf.buf);
		
		emn_call((struct emn_object *)cli, NULL, EMN_EV_RECV, &len);
	} else if (len == 0) {
		/* Orderly shutdown of the socket, try flushing output. */
		printf("peer closed\n");
		emn_client_destroy(cli);
	} else {
		perror("read");
		emn_client_destroy(cli);
	}
}

static void ev_write_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	
}

static void ev_accept_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	int sock = -1;
	struct sockaddr_in sin;
	socklen_t sin_len = sizeof(sin);
	struct emn_server *srv = (struct emn_server *)w->data;
	struct emn_client *cli = NULL;
	
	sock = accept(w->fd, (struct sockaddr *)&sin, &sin_len);
	if (sock < 0) {
		perror("socket");
		return;
	}

	cli = calloc(1, sizeof(struct emn_client));
	if (!cli) {
		close(sock);
		return;
	}
	
	cli->type = EMN_TYPE_CLIENT;
	cli->handler = srv->handler;
	cli->proto_handler = srv->proto_handler;
	cli->srv = srv;
	cli->sock = sock;
	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->ior.data = cli;
	
	printf("%p conn from %s:%d\n", cli, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	
	emn_call((struct emn_object *)srv, NULL, EMN_EV_ACCEPT, &sin);
}

static void emn_http_handler(struct emn_object *obj, int event, void *data)
{
	struct emn_server *srv = (struct emn_server *)obj;
	printf("emn_http_handler:%p\n", srv);
}

void emn_set_protocol_http_websocket(struct emn_server *srv)
{
	srv->proto_handler = emn_http_handler;
}

struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler)
{
	struct sockaddr_in sin;
	struct emn_server *srv = NULL;
	int proto;
	
	if (parse_address(address, &sin, &proto) <= 0) {
		printf("cannot parse address\n");
		return NULL;
	}

	srv = calloc(1, sizeof(struct emn_server));
	if (!srv)
		return NULL;
	
	INIT_LIST_HEAD(&srv->client_list);
	
	srv->sock = open_listening_socket(&sin, proto, 0);
	if (srv->sock < 0) {
		printf("cannot open_listening_socket\n");
		emn_server_destroy(srv);
		return NULL;
	}
	
	srv->type = EMN_TYPE_SERVER;
	srv->handler = ev_handler;
	srv->loop = loop;
	
	ev_io_init(&srv->ior, ev_accept_cb, srv->sock, EV_READ);
	srv->ior.data = srv;
	ev_io_start(loop, &srv->ior);
  
	return srv;
}

void emn_server_destroy(struct emn_server *srv)
{
	if (srv) {
		struct emn_client *cli, *tmp;
		
		if (srv->sock > 0)
			close(srv->sock);
		
		ev_io_stop(srv->loop, &srv->ior);
		
		list_for_each_entry_safe(cli, tmp, &srv->client_list, list) {
			emn_client_destroy(cli);
		}
		
		free(srv);
	}
}

void emn_client_destroy(struct emn_client *cli)
{
	if (cli) {
		if (cli->sock > 0)
			close(cli->sock);
		
		ebuf_free(&cli->rbuf);
		ebuf_free(&cli->sbuf);
		
		ev_io_stop(cli->srv->loop, &cli->ior);
		ev_io_stop(cli->srv->loop, &cli->iow);
		
		list_del(&cli->list);
		free(cli);
	}
}