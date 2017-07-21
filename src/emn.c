#include "emn.h"
#include "list.h"

/* HTTP message */
struct http_message {
	/* HTTP Request line (or HTTP response line) */
	struct emn_str uri;    /* "/my_file.html" */

	/* Headers */
	struct emn_str header_names[EMN_MAX_HTTP_HEADERS];
	struct emn_str header_values[EMN_MAX_HTTP_HEADERS];

	/* Message body */
	struct emn_str body; /* Zero-length for requests with no body */
};

struct emn_server {
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	ev_io ior;
	uint8_t flags;
	struct ev_loop *loop;
	struct list_head client_list;
};

struct emn_client {
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	ev_io ior;
	ev_io iow;
	struct ebuf rbuf;	/* recv buf */
	struct ebuf sbuf;	/* send buf */
	void *data;			/* Pointing to protocol related structures */
	struct emn_server *srv;
	struct list_head list;
};

inline struct ebuf *emn_get_rbuf(struct emn_client *cli)
{
	return &cli->rbuf;
}

inline struct ebuf *emn_get_sbuf(struct emn_client *cli)
{
	return &cli->sbuf;
}

inline static void emn_call(struct emn_client *cli, int event, void *data)
{
	emn_event_handler_t handler = cli->proto_handler ? cli->proto_handler : cli->handler;

	if (handler)
		handler(cli, event, data);
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
		emn_call(cli, EMN_EV_RECV, &len);
	} else if (len == 0) {
		/* Orderly shutdown of the socket, try flushing output. */
		emn_call(cli, EMN_EV_CLOSE, NULL);
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
	
	cli->handler = srv->handler;
	cli->proto_handler = srv->proto_handler;
	cli->srv = srv;
	cli->sock = sock;

	if (srv->flags & EMN_FLAGS_HTTP) {
		cli->data = calloc(1, sizeof(http_parser));
		http_parser_init((http_parser *)cli->data, HTTP_REQUEST);
		((http_parser *)cli->data)->data = cli;
	}
	
	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->ior.data = cli;

	emn_call(cli, EMN_EV_ACCEPT, &sin);
}

int on_url(http_parser *parser, const char *at, size_t len)
{	
	struct emn_client *cli = (struct emn_client *)parser->data;
	
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t len)
{
	printf("proto: %d.%d\n", parser->http_major, parser->http_minor);
	printf("Header field: %.*s\n", (int)len, at);
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t len)
{
	printf("Header value: %.*s\n", (int)len, at);
    return 0;
}

int on_headers_complete(http_parser *parser)
{
	printf("\n***HEADERS COMPLETE***\n\n");
	return 0;
}

int on_body(http_parser *parser, const char *at, size_t len)
{
	printf("Body: %.*s\n", (int)len, at);
    return 0;
}

int on_message_complete(http_parser *parser)
{
	printf("\n***MESSAGE COMPLETE***\n\n");
	return 0;
}

static http_parser_settings parser_settings =
{
	.on_url              = on_url,
	.on_header_field     = on_header_field,
	.on_header_value     = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_body             = on_body,
	.on_message_complete = on_message_complete
};

static void emn_http_handler(struct emn_client *cli, int event, void *data)
{
	cli->handler(cli, event, data);

	if (event == EMN_EV_RECV) {
		size_t nparsed;
		nparsed = http_parser_execute((http_parser *)cli->data, &parser_settings, cli->rbuf.buf, cli->rbuf.len);
		printf("nparsed = %zu\n", nparsed);
	}
}

void emn_set_protocol_http(struct emn_server *srv)
{
	srv->proto_handler = emn_http_handler;
	srv->flags |= EMN_FLAGS_HTTP;
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
