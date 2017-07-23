#include "emn.h"
#include "emn_internal.h"
#include "list.h"
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
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
	ssize_t len;
	static char *buf;
	struct emn_client *cli = (struct emn_client *)w->data;
	
	if (!buf) {
		buf = malloc(EMN_TCP_RECV_BUFFER_SIZE);
		if (!buf) {
			syslog(LOG_ERR, "emn: can't alloc mem");
			return;
		}
	}
	
	len = read(w->fd, buf, EMN_TCP_RECV_BUFFER_SIZE);
	if (len > 0) {
		ebuf_append(&cli->rbuf, buf, len);
		emn_call(cli, NULL, EMN_EV_RECV, &len);
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
		syslog(LOG_ERR, "emn: accept failed:%s", strerror(errno));
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

void emn_send(struct emn_client *cli, const void *buf, int len)
{
	ebuf_append(&cli->sbuf, buf, len);
    ev_io_start(cli->srv->loop, &cli->iow);
}

int emn_printf(struct emn_client *cli, const char *fmt, ...)
{
	int len;
	va_list ap;
	char buf[1024] = "";
	
	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	emn_send(cli, buf, len);
	
	return len;
}

struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler)
{
	struct sockaddr_in sin;
	struct emn_server *srv = NULL;
	int proto;
	
	if (emn_parse_address(address, &sin, &proto) < 0) {
		syslog(LOG_ERR, "emn: can't parse address");
		return NULL;
	}

	srv = calloc(1, sizeof(struct emn_server));
	if (!srv) {
		syslog(LOG_ERR, "emn: can't alloc mem");
		return NULL;
	}
	
	INIT_LIST_HEAD(&srv->client_list);
	
	srv->sock = emn_open_listening_socket(&sin, proto, 0);
	if (srv->sock < 0) {
		syslog(LOG_ERR, "emn: can't open listening_socket");
		emn_server_destroy(srv);
		return NULL;
	}
	
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
		
		list_del(&cli->list);
		free(cli);
	}
}

void emn_sever_set_proto_handler(struct emn_server *srv, emn_event_handler_t proto_handler)
{
	assert(srv);
	assert(proto_handler);
	srv->proto_handler = proto_handler;
}

void emn_sever_set_flags(struct emn_server *srv, uint16_t flags)
{
	assert(srv);
	srv->flags |= flags;
}

void emn_sever_set_opts(struct emn_server *srv, void *opts)
{
	assert(srv);
	srv->opts = opts;
}

void *emn_server_get_opts(struct emn_server *srv)
{
	return srv->opts;
}

void emn_client_set_flags(struct emn_client *cli, uint16_t flags)
{
	assert(cli);
	cli->flags |= flags;
}

void emn_client_set_send_fd(struct emn_client *cli, int fd)
{
	cli->send_fd = fd;
}

void emn_client_set_data(struct emn_client *cli, void *data)
{
	cli->data = data;
}

void *emn_client_get_data(struct emn_client *cli)
{
	return cli->data;
}

struct emn_server *emn_client_get_server(struct emn_client *cli)
{
	return cli->srv;
}

emn_event_handler_t emn_client_get_handler(struct emn_client *cli)
{
	return cli->handler;
}

