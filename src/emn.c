#include "emn.h"
#include "list.h"
#include "emn_internal.h"
#include <sys/sendfile.h>

inline struct ebuf *emn_get_rbuf(struct emn_client *cli)
{
	return &cli->rbuf;
}

inline struct ebuf *emn_get_sbuf(struct emn_client *cli)
{
	return &cli->sbuf;
}

inline int emn_call(struct emn_client *cli, emn_event_handler_t handler, int event, void *data)
{
	if (!handler)
		handler = cli->proto_handler ? cli->proto_handler : cli->handler;
	
	if (handler)
		return handler(cli, event, data);
	return 0;
}

static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	struct ebuf ebuf;
	ssize_t len;
	struct emn_client *cli = (struct emn_client *)w->data;

	ebuf_init(&ebuf, EMN_RECV_BUFFER_SIZE);
	
	len = read(w->fd, ebuf.buf, ebuf.size);
	if (len > 0) {
		ebuf.len = len;
		emn_call(cli, NULL, EMN_EV_RECV, &ebuf);
	} else if (len == 0) {
		/* Orderly shutdown of the socket, try flushing output. */
		cli->flags |= EMN_FLAGS_SEND_AND_CLOSE;
	} else {
		cli->flags |= EMN_FLAGS_CLOSE_IMMEDIATELY;
	}

	ev_io_start(cli->srv->loop, &cli->iow);
}

static void ev_write_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	struct emn_client *cli = (struct emn_client *)w->data;
	struct ebuf *sbuf = &cli->sbuf;
	ssize_t len = -1;

	len = write(w->fd, sbuf->buf, sbuf->len);
	if (len > 0)
		ebuf_remove(sbuf, len);

	if (cli->flags & EMN_FLAGS_CLOSE_IMMEDIATELY) {
		emn_client_destroy(cli);
		return;
	}

	if (sbuf->len == 0) {
		ev_io_stop(cli->srv->loop, &cli->iow);
		
		if (cli->send_fd > 0) {
			struct stat st;
			
			fstat(cli->send_fd, &st);
			sendfile(w->fd, cli->send_fd, NULL, st.st_size);		
			close(cli->send_fd);
			cli->send_fd = -1;
		}

		if (cli->flags & EMN_FLAGS_SEND_AND_CLOSE)
			emn_client_destroy(cli);
	}
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
		emn_log(LOG_ERR, "emn: accept failed:%s", strerror(errno));
		return;
	}

	cli = calloc(1, sizeof(struct emn_client));
	if (!cli) {
		close(sock);
		return;
	}
	
	cli->handler = srv->handler;
	cli->proto_handler = srv->proto_handler;
	cli->srv = srv;
	cli->sock = sock;
	
	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->iow.data = cli;

	emn_call(cli, NULL, EMN_EV_ACCEPT, &sin);
}

static int parse_address(const char *address, struct sockaddr_in *sin, int *proto)
{
	const char *str;
	char *p;
	char host[16] = "";
	uint16_t port = 0;

	assert(address);
	
	memset(sin, 0, sizeof(struct sockaddr_in));
	
	sin->sin_family = AF_INET;
	*proto = SOCK_STREAM;
	str = address;
	
	if (strncmp(str, "udp://", 6) == 0) {
		str += 6;
		*proto = SOCK_DGRAM;
	} else if (strncmp(str, "tcp://", 6) == 0) {
		str += 6;
	}

	p = strchr(str, ':');
	if (p) {
		if (p - str > 15)
			return -1;
		
		memcpy(host, str, p - str);

		if (strcmp(host, "*")) {	
			if (inet_pton(AF_INET, host, &sin->sin_addr) <= 0)
				return -1;
		}
		str = p + 1;
	}

	port = atoi(str);
	if (port <= 0)
		return -1;

	sin->sin_port = htons(port);
	
	return 0;
}


size_t emn_send(struct emn_client *cli, const void *buf, int len)
{
	len = ebuf_append(&cli->sbuf, buf, len);
	if (len > 0)
	    ev_io_start(cli->srv->loop, &cli->iow);
	return len;
}

int emn_printf(struct emn_client *cli, const char *fmt, ...)
{
	int len = 0;
	va_list ap;
	char *str = NULL;

	assert(fmt);

	if (*fmt) {
		va_start(ap, fmt);
		len = vasprintf(&str, fmt, ap);
		va_end(ap);
	}
	
	if (len >= 0) {
		len = emn_send(cli, str, len);
		free(str);
	}
	return len;
}

struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler)
{
	return emn_bind_opt(loop, address, ev_handler, NULL);
}

struct emn_server *emn_bind_opt(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler, 
								struct emn_bind_opts *opts)
{
	struct sockaddr_in sin;
	struct emn_server *srv = NULL;
	int sock;
	int proto;
	int on = 1;
	
	if (parse_address(address, &sin, &proto) < 0) {
		emn_log(LOG_ERR, "parse address failed");
		return NULL;
	}

	srv = calloc(1, sizeof(struct emn_server));
	if (!srv) {
		emn_log(LOG_ERR, "alloc mem failed");
		return NULL;
	}
	
	INIT_LIST_HEAD(&srv->client_list);

	sock = socket(sin.sin_family, proto | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (sock < 0) {
		emn_log(LOG_ERR, "can't create socket:%s", strerror(errno));
		goto err;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0) {
		emn_log(LOG_ERR, "can't bind:%s", strerror(errno));
		goto err;
	}

	if (proto == SOCK_STREAM && listen(sock, SOMAXCONN) < 0) {
		emn_log(LOG_ERR, "can't listening:%s", strerror(errno));
		goto err;
	}

	srv->sock = sock;
	srv->handler = ev_handler;
	srv->loop = loop;
	
	ev_io_init(&srv->ior, ev_accept_cb, sock, EV_READ);
	srv->ior.data = srv;
	ev_io_start(loop, &srv->ior);
  
	return srv;
err:
	emn_server_destroy(srv);
	return NULL;
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

		if (srv->opts)
			free(srv->opts);
		
		free(srv);
	}
}

void emn_client_destroy(struct emn_client *cli)
{
	if (cli) {
		emn_call(cli, NULL, EMN_EV_CLOSE, NULL);
		
		if (cli->sock > 0)
			close(cli->sock);

		if (cli->data)
			free(cli->data);
		
		ebuf_free(&cli->rbuf);
		ebuf_free(&cli->sbuf);
		
		ev_io_stop(cli->srv->loop, &cli->ior);
		ev_io_stop(cli->srv->loop, &cli->iow);
		ev_timer_stop(cli->srv->loop, &cli->timer);
		
		list_del(&cli->list);
		free(cli);
	}
}

void emn_client_set_flags(struct emn_client *cli, uint16_t flags)
{
	assert(cli);
	cli->flags |= flags;
}

