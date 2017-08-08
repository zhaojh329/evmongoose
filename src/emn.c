#include "emn.h"
#include "list.h"
#include "emn_ssl.h"
#include "emn_internal.h"
#include <sys/mman.h>

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

	if (cli->flags & EMN_FLAGS_SSL)
#if (EMN_USE_OPENSSL)		
		len = SSL_read(cli->ssl, ebuf.buf, ebuf.size);
#elif (EMN_USE_CYASSL)
		len = wolfSSL_read(cli->ssl, ebuf.buf, ebuf.size);
#endif
	else
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

	if (cli->flags & EMN_FLAGS_CONNECTING) {
		int err = 0;
		socklen_t slen = sizeof(err);
		int ret = getsockopt(w->fd, SOL_SOCKET, SO_ERROR, (char *) &err, &slen);
		if (ret != 0) {
			err = 1;
		} else if (err == EAGAIN || err == EWOULDBLOCK) {
			err = 0;
		}
		
		cli->flags &= ~EMN_FLAGS_CONNECTING;
		emn_call(cli, NULL, EMN_EV_CONNECT, &err);
		if (err)
			emn_client_destroy(cli);
		ev_io_stop(loop, &cli->iow);
		return;
	}

	if (cli->flags & EMN_FLAGS_SSL)
#if (EMN_USE_OPENSSL)		
		len = SSL_write(cli->ssl, sbuf->buf, sbuf->len);
#elif (EMN_USE_CYASSL)
		len = wolfSSL_write(cli->ssl, sbuf->buf, sbuf->len);
#endif
	else
		len = write(w->fd, sbuf->buf, sbuf->len);
	
	if (len > 0)
		ebuf_remove(sbuf, len);

	if (len < 0) {
		emn_client_destroy(cli);
		return;
	}

	if (cli->flags & EMN_FLAGS_CLOSE_IMMEDIATELY) {
		emn_client_destroy(cli);
		return;
	}

	if (sbuf->len == 0) {
		ev_io_stop(cli->srv->loop, &cli->iow);
		
		if (cli->send_fd > 0) {
			struct stat st;
			char *fb = NULL;
		
			fstat(cli->send_fd, &st);
			
			fb = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, cli->send_fd, 0);
			close(cli->send_fd);
			cli->send_fd = -1;
			
			if (cli->flags & EMN_FLAGS_SSL) {
#if (EMN_USE_OPENSSL)				
				if (SSL_write(cli->ssl, fb, st.st_size) < 0) {
#elif (EMN_USE_CYASSL)
				if (wolfSSL_write(cli->ssl, fb, st.st_size) < 0) {
#endif					
					emn_log(LOG_ERR, "SSL_write failed");
					cli->flags |= EMN_FLAGS_CLOSE_IMMEDIATELY;
				}
			} else {
				if (write(w->fd, fb, st.st_size) < 0) {
					emn_log(LOG_ERR, "write failed");
					cli->flags |= EMN_FLAGS_CLOSE_IMMEDIATELY;
				}
			}
		}

		if ((cli->flags & EMN_FLAGS_SEND_AND_CLOSE) || (cli->flags & EMN_FLAGS_CLOSE_IMMEDIATELY))
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
	cli->loop = loop;

#if (EMN_SSL_ENABLED)
	if (srv->flags & EMN_FLAGS_SSL) {
#if (EMN_USE_OPENSSL)		
		cli->ssl = SSL_new(srv->ssl_ctx);
#elif (EMN_USE_CYASSL)
		cli->ssl = wolfSSL_new(srv->ssl_ctx);
#endif
		if (!cli->ssl) {
			emn_client_destroy(cli);
			return;
		}
#if (EMN_USE_OPENSSL)
		SSL_set_fd(cli->ssl, cli->sock);
#elif (EMN_USE_CYASSL)
		wolfSSL_set_fd(cli->ssl, cli->sock);
#endif
		cli->flags |= EMN_FLAGS_SSL;
#if (EMN_USE_OPENSSL)		
		if (!SSL_accept(cli->ssl)) {

			emn_log(LOG_ERR, "SSL_accept failed");
			emn_client_destroy(cli);
			return;
		}
#endif		
	}
#endif

	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->iow.data = cli;

	emn_call(cli, NULL, EMN_EV_ACCEPT, &sin);
}

/*
 * Address format: [PROTO://][HOST]:PORT
 *
 * HOST could be IPv4/IPv6 address or a host name.
 * `host` is a destination buffer to hold parsed HOST part.
 * `proto` is a returned socket type, either SOCK_STREAM or SOCK_DGRAM
 *
 * Return:
 *   -1	on parse error
 *   0	if HOST needs DNS lookup
 *   1	on success
 */
static int parse_address(const char *address, struct sockaddr_in *sin,
					int *proto, char *host, size_t host_len)
{
	int ret = 0;
	char *p;
	const char *str;
	uint16_t port = 0;

	assert(address);

	memset(host, 0, host_len);
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
		if (p - str > host_len)
			return -1;
		
		memcpy(host, str, p - str);

		if (strcmp(host, "*")) {	
			ret = inet_pton(AF_INET, host, &sin->sin_addr);
			if (ret < 0)
				return -1;
		}
		str = p + 1;
	}

	port = atoi(str);
	if (port <= 0)
		return -1;

	sin->sin_port = htons(port);
	
	return ret;
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
	int ret = -1;
	struct sockaddr_in sin;
	struct emn_server *srv = NULL;
	int sock;
	int proto;
	int on = 1;
	char host[128];

	ret = parse_address(address, &sin, &proto, host, sizeof(host));
	if (ret <= 0) {
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

	if (proto == SOCK_DGRAM)
		srv->flags |= EMN_FLAGS_UDP;

#if (EMN_SSL_ENABLED)
	if (proto == SOCK_STREAM && opts && opts->ssl_cert) {
#if (EMN_USE_OPENSSL)
		SSL_CTX *ctx = NULL;
#elif (EMN_USE_CYASSL)
		WOLFSSL_CTX *ctx = NULL;
#endif
		ctx = emn_ssl_init(opts->ssl_cert, opts->ssl_key, EMN_TYPE_SERVER);
		if (!ctx)
			goto err;

		srv->ssl_ctx = ctx;
		srv->flags |= EMN_FLAGS_SSL;
	}
#endif	
	
	ev_io_init(&srv->ior, ev_accept_cb, sock, EV_READ);
	srv->ior.data = srv;
	ev_io_start(loop, &srv->ior);
  
	return srv;
err:
	emn_server_destroy(srv);
	return NULL;
}

static struct emn_client *emn_do_connect(struct emn_client *cli)
{
	int sock;

	sock = socket(cli->sin.sin_family,
			(cli->flags & EMN_FLAGS_UDP ? SOCK_DGRAM : SOCK_STREAM) | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (sock < 0) {
		emn_log(LOG_ERR, "can't create socket:%s", strerror(errno));
		goto err;
	}

	if (connect(sock, (struct sockaddr *)&cli->sin, sizeof(cli->sin)) < 0) {
		if (errno != EINPROGRESS) {
			emn_log(LOG_ERR, "can't connect:%s", strerror(errno));
			goto err;
		}
	}

	cli->flags |= EMN_FLAGS_CONNECTING;

	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(cli->loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->iow.data = cli;
	ev_io_start(cli->loop, &cli->iow);

	return cli;
err:
	emn_client_destroy(cli);
	return NULL;	
}

static void resolve_handler(int status, struct hostent *host, void *data)
{
	char **p;

	struct emn_client *cli = (struct emn_client *)data;
	
	cli->flags &= ~EMN_FLAGS_RESOLVING;
	
	if (status == EMN_ARES_TIMEOUT) {
		//: TODO
		printf("resolve timeout\n");
		return;
	}

	for (p = host->h_addr_list; *p; p++) {
		memcpy(&cli->sin.sin_addr, *p, sizeof(struct in_addr));
		emn_do_connect(cli);
		return;
	}
}

struct emn_client *emn_connect(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler)
{
	int ret = -1;
	struct emn_client *cli = NULL;
	int proto;
	char host[128];

	cli = calloc(1, sizeof(struct emn_client));
	if (!cli) {
		emn_log(LOG_ERR, "alloc mem failed");
		return NULL;
	}

	cli->loop = loop;
	cli->handler = ev_handler;

	ret = parse_address(address, &cli->sin, &proto, host, sizeof(host));
	if (ret < 0) {
		emn_log(LOG_ERR, "parse address failed");
		goto err;
	}

	if (proto == SOCK_DGRAM)
		cli->flags |=  EMN_FLAGS_UDP;

	if (ret == 0) {
		cli->flags |= EMN_FLAGS_RESOLVING;
		emn_resolve_single(loop, host, resolve_handler, cli);
		return cli;
	}
	
	return emn_do_connect(cli);	
err:
	emn_client_destroy(cli);
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

#if (EMN_SSL_ENABLED)
		if (srv->flags & EMN_FLAGS_SSL) {
#if (EMN_USE_OPENSSL)			
			SSL_CTX_free(srv->ssl_ctx);
#elif (EMN_USE_CYASSL)
			wolfSSL_CTX_free(srv->ssl_ctx);
#endif
		}
#endif
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

#if (EMN_SSL_ENABLED)		
		if (cli->flags & EMN_FLAGS_SSL)
#if (EMN_USE_OPENSSL)			
			SSL_free(cli->ssl);
#elif (EMN_USE_CYASSL)
			wolfSSL_free(cli->ssl);
#endif
#endif

		ebuf_free(&cli->rbuf);
		ebuf_free(&cli->sbuf);
		
		ev_io_stop(cli->loop, &cli->ior);
		ev_io_stop(cli->loop, &cli->iow);
		ev_timer_stop(cli->loop, &cli->timer);

		if (cli->srv)
			list_del(&cli->list);
		free(cli);
	}
}

void emn_client_set_flags(struct emn_client *cli, uint16_t flags)
{
	assert(cli);
	cli->flags |= flags;
}

