#include "emn.h"
#include "list.h"
#include <errno.h>
#include <stdarg.h>
#include <sys/sendfile.h>

/* HTTP message */
struct http_message {
	http_parser parser;

	struct emn_str url;
	struct emn_str path;
	struct emn_str query;

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
	uint16_t flags;
	void *opts;		/* Pointing to protocol related structures */
	struct ev_loop *loop;
	struct list_head client_list;
};

struct emn_client {
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	int send_fd;	/* File descriptor of File to send */
	ev_io ior;
	ev_io iow;
	uint16_t flags;
	struct ebuf rbuf;	/* recv buf */
	struct ebuf sbuf;	/* send buf */
	void *data;			/* Pointing to protocol related structures */
	struct emn_server *srv;
	struct list_head list;
};

inline void emn_str_init(struct emn_str *str, const char *at, size_t len)
{
	str->p = at;
	str-> len = len;
}

inline struct ebuf *emn_get_rbuf(struct emn_client *cli)
{
	return &cli->rbuf;
}

inline struct ebuf *emn_get_sbuf(struct emn_client *cli)
{
	return &cli->sbuf;
}

inline enum http_method emn_get_http_method(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return hm->parser.method;
}

inline struct emn_str *emn_get_http_url(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return &hm->url;
}

inline struct emn_str *emn_get_http_path(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return &hm->path;
}

inline struct emn_str *emn_get_http_query(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return &hm->query;
}

inline uint8_t emn_get_http_version_major(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return hm->parser.http_major;
}

inline uint8_t emn_get_http_version_minor(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return hm->parser.http_minor;
}

inline struct emn_str *emn_get_http_header(struct emn_client *cli, const char *name)
{
	int i = 0;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_names = hm->header_names;
	struct emn_str *header_values = hm->header_values;
	while (i < EMN_MAX_HTTP_HEADERS) {
		if (!strncasecmp(header_names[i].p, name, header_names[i].len)) {
			return header_values + i;
			break;
		}
		i++;
	}
	return NULL;
}

inline struct emn_str *emn_get_http_body(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return &hm->body;
}

inline static int emn_call(struct emn_client *cli, int event, void *data)
{
	emn_event_handler_t handler = cli->proto_handler ? cli->proto_handler : cli->handler;
	
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
	struct emn_client *cli = (struct emn_client *)w->data;
	struct ebuf *sbuf = &cli->sbuf;
	ssize_t len = write(w->fd, sbuf->buf, sbuf->len);
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
		struct http_message *hm = calloc(1, sizeof(struct http_message));
		http_parser_init(&hm->parser, HTTP_REQUEST);
		hm->parser.data = cli;
		cli->data = hm;
	}
	
	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->iow.data = cli;

	emn_call(cli, EMN_EV_ACCEPT, &sin);
}

static const char *emn_http_status_message(int code)
{
	switch (code) {
	case 301:
		return "Moved";
	case 302:
		return "Found";
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 416:
		return "Requested Range Not Satisfiable";
	case 418:
		return "I'm a teapot";
	case 500:
		return "Internal Server Error";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	default:
		return "OK";
	}
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

void emn_send_http_response_line(struct emn_client *cli, int code, const char *extra_headers)
{
	emn_printf(cli, "HTTP/1.1 %d %s\r\nServer: Emn %d.%d\r\n", code, emn_http_status_message(code), 
		EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	if (extra_headers)
		emn_printf(cli, "%s\r\n", extra_headers);
}

void emn_send_http_head(struct emn_client *cli, int code, ssize_t content_length, const char *extra_headers)
{
	emn_send_http_response_line(cli, code, extra_headers);
	if (content_length < 0)
		emn_printf(cli, "%s", "Transfer-Encoding: chunked\r\n");
	else
		emn_printf(cli, "Content-Length: %zd\r\n", content_length);
	emn_send(cli, "\r\n", 2);
}

void emn_send_http_error(struct emn_client *cli, int code, const char *reason)
{
	if (!reason)
		reason = emn_http_status_message(code);

	emn_send_http_head(cli, code, strlen(reason), "Content-Type: text/plain\r\nConnection: close");
	emn_send(cli, reason, strlen(reason));
	cli->flags |= EMN_FLAGS_SEND_AND_CLOSE;
}

static void emn_send_http_file(struct emn_client *cli)
{
	struct emn_server *srv = cli->srv;
	struct http_opts *opts = (struct http_opts *)srv->opts;
	struct http_message *hm = (struct http_message *)cli->data;
	const char *document_root = opts ? (opts->document_root ? opts->document_root : ".") : ".";
	char *local_path = calloc(1, strlen(document_root) + hm->path.len + 1);
	int code = 200;
	
	strcpy(local_path, document_root);
	memcpy(local_path + strlen(document_root), hm->path.p, hm->path.len);

	cli->send_fd = open(local_path, O_RDONLY);
	if (cli->send_fd < 0) {
		switch (errno) {
		case EACCES:
			code = 403;
	        break;
		case ENOENT:
	        code = 404;
	        break;
	      default:
	        code = 500;
		}
	}

	if (cli->send_fd > 0) {
		struct stat st;
		
		fstat(cli->send_fd, &st);

		if (S_ISDIR(st.st_mode)) {
			close (cli->send_fd);
			cli->send_fd = -1;
			code = 404;
			goto end;
		}

		emn_send_http_response_line(cli, code, NULL);
    	emn_printf(cli,
              "Content-Type: text/html\r\n"
              "Content-Length: %zu\r\n"
              "Connection: %s\r\n"
              "\r\n",
              st.st_size, http_should_keep_alive(&hm->parser) ? "keep-alive" : "close"
              );
	}

	if (code != 200)
		emn_send_http_error(cli, code, NULL);

	ebuf_remove(&cli->rbuf, cli->rbuf.len);
	
end:
	free(local_path);
}

static void emn_serve_http(struct emn_client *cli)
{
	emn_send_http_file(cli);
}

int on_url(http_parser *parser, const char *at, size_t len)
{	
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct http_parser_url url;

	emn_str_init(&hm->url, at, len);
	
	if (http_parser_parse_url(at, len, 0, &url))
		printf("http_parser_parse_url failed\n");
	else {
		if (url.field_set & (1 << UF_PATH))
			emn_str_init(&hm->path, at + url.field_data[UF_PATH].off, url.field_data[UF_PATH].len);

		if (url.field_set & (1 << UF_QUERY))
			emn_str_init(&hm->query, at + url.field_data[UF_QUERY].off, url.field_data[UF_QUERY].len);
	}
		
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t len)
{
	int i = 0;
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_names = hm->header_names;
	
	while (i < EMN_MAX_HTTP_HEADERS) {
		if (header_names[i].len == 0) {
			emn_str_init(header_names + i, at, len);
			break;
		}
		i++;
	}
	
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t len)
{
	int i = 0;
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_values = hm->header_values;
	
	while (i < EMN_MAX_HTTP_HEADERS) {
		if (header_values[i].len == 0) {
			emn_str_init(header_values + i, at, len);
			break;
		}
		i++;
	}
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t len)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	emn_str_init(&hm->body, at, len);
    return 0;
}

int on_message_complete(http_parser *parser)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
#if 0
	int i = 0;
	printf("\n***MESSAGE COMPLETE***\n\n");
	printf("method: %s\n", http_method_str(parser->method));
	printf("uri: %.*s\n", (int)hm->uri.len, hm->uri.p);
	printf("proto: %d.%d\n", parser->http_major, parser->http_minor);

	while (i < EMN_MAX_HTTP_HEADERS) {
		if (hm->header_names[i].len > 0 && hm->header_values[i].len > 0) {
			printf("%.*s: %.*s\n", (int)hm->header_names[i].len, hm->header_names[i].p, 
				(int)hm->header_values[i].len, hm->header_values[i].p);
			i++;
		} else {
			break;
		}
	}

	printf("body:%.*s\n", (int)hm->body.len, hm->body.p);
#endif

	if (!http_should_keep_alive(&hm->parser))
		cli->flags |= EMN_FLAGS_SEND_AND_CLOSE;
		
	if (!emn_call(cli, EMN_EV_HTTP_REQUEST, hm))
		emn_serve_http(cli);
	
	return 0;
}

static http_parser_settings parser_settings =
{
	.on_url              = on_url,
	.on_header_field     = on_header_field,
	.on_header_value     = on_header_value,
	.on_body             = on_body,
	.on_message_complete = on_message_complete
};

static int emn_http_handler(struct emn_client *cli, int event, void *data)
{
	cli->handler(cli, event, data);

	if (event == EMN_EV_RECV) {
		size_t nparsed;
		struct http_message *hm = (struct http_message *)cli->data;
		nparsed = http_parser_execute(&hm->parser, &parser_settings, cli->rbuf.buf, cli->rbuf.len);
		printf("nparsed = %zu\n", nparsed);
	}
	
	return 0;
}

void emn_set_protocol_http(struct emn_server *srv, struct http_opts *opts)
{
	srv->proto_handler = emn_http_handler;
	srv->flags |= EMN_FLAGS_HTTP;
	srv->opts = opts;
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

		if (srv->opts)
			free(srv->opts);
		
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
