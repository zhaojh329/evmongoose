#include "emn_http.h"
#include "emn.h"
#include "emn_internal.h"
#include <errno.h>

/* HTTP message */
struct http_message {
	http_parser parser;

	struct emn_str url;
	struct emn_str path;
	struct emn_str query;

	/* Headers */
	uint8_t nheader;	/* Number of headers */
	struct emn_str header_names[EMN_MAX_HTTP_HEADERS];
	struct emn_str header_values[EMN_MAX_HTTP_HEADERS];

	/* Message body */
	struct emn_str body; /* Zero-length for requests with no body */
};

static struct {
	int code;
	const char *reason;
} http_status_message[] = {
	{200, "OK"},
	{301, "Moved"},
	{302, "Found"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{500, "Internal Server Error"},
	{501, "Not Implemented"},
	{502, "Bad Gateway"},
	{503, "Service Unavailable"},
	{-1, NULL}
};

static const struct {
	const char *extension;
	const char *type;
} http_mime_types[] = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"shtm", "text/html"},
    {"shtml", "text/html"},
    {"css", "text/css"},
    {"js", "application/x-javascript"},
    {"ico", "image/x-icon"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"txt", "text/plain"},
    {"torrent", "application/x-bittorrent"},
    {"wav", "audio/x-wav"},
    {"mp3", "audio/x-mp3"},
    {"mid", "audio/mid"},
    {"m3u", "audio/x-mpegurl"},
    {"ogg", "application/ogg"},
    {"ram", "audio/x-pn-realaudio"},
    {"xml", "text/xml"},
    {"ttf", "application/x-font-ttf"},
    {"json", "application/json"},
    {"xslt", "application/xml"},
    {"xsl", "application/xml"},
    {"ra", "audio/x-pn-realaudio"},
    {"doc", "application/msword"},
    {"exe", "application/octet-stream"},
    {"zip", "application/x-zip-compressed"},
    {"xls", "application/excel"},
    {"tgz", "application/x-tar-gz"},
    {"tar", "application/x-tar"},
    {"gz", "application/x-gunzip"},
    {"arj", "application/x-arj-compressed"},
    {"rar", "application/x-rar-compressed"},
    {"rtf", "application/rtf"},
    {"pdf", "application/pdf"},
    {"swf", "application/x-shockwave-flash"},
    {"mpg", "video/mpeg"},
    {"webm", "video/webm"},
    {"mpeg", "video/mpeg"},
    {"mov", "video/quicktime"},
    {"mp4", "video/mp4"},
    {"m4v", "video/x-m4v"},
    {"asf", "video/x-ms-asf"},
    {"avi", "video/x-msvideo"},
    {"bmp", "image/bmp"},
    {NULL, NULL}
};

static const char *get_http_status_message(int code)
{
	int i;
	const char *reason = "OK";

	for (i = 0; http_status_message[i].reason; i++) {
		if (code == http_status_message[i].code)
			reason = http_status_message[i].reason;
	}
	return reason;
}

static void emh_http_construct_etag(char *buf, size_t buf_len, struct stat *st)
{
	snprintf(buf, buf_len, "\"%zx.%zu\"", st->st_mtime, st->st_size);
}

static int emn_http_is_not_modified(struct emn_client *cli, struct stat *st)
{
	struct emn_str *hdr;
	
	if ((hdr = emn_get_http_header(cli, "If-None-Match"))) {
		char etag[64] = "";
		emh_http_construct_etag(etag, sizeof(etag), st);
		return emn_strvcasecmp(hdr, etag) == 0;
	} else if ((hdr = emn_get_http_header(cli, "If-Modified-Since"))) {
		return st->st_mtime <= emn_gmt2time(hdr->p);
	} else {
		return 0;
	}
}

static const char *get_mime_type(const struct emn_str *path, const char *dft)
{
	int i;
	char *p;
	struct emn_str ext;

	p = memrchr(path->p, '.', path->len);
	if (!p || !*p)
		return dft;

	emn_str_init(&ext, p + 1, path->len + path->p - p - 1);

	for (i = 0; http_mime_types[i].extension; i++) {
		if (!emn_strvcasecmp(&ext, http_mime_types[i].extension))
			return http_mime_types[i].type;
	}
	return dft;
}

static void send_http_file(struct emn_client *cli, struct http_message *hm)
{
	struct emn_server *srv = cli->srv;
	struct http_opts *opts = (struct http_opts *)srv->opts;
	const char *document_root = opts ? (opts->document_root ? opts->document_root : ".") : ".";
	const char *index_files = opts ? (opts->index_files ? opts->index_files : "/index.html") : "/index.html";
	char *local_path = calloc(1, strlen(document_root) + hm->path.len + 1);
	int code = 200;
	int send_fd = -1;

	if (!strncmp(hm->path.p, "/", hm->path.len)) {
		emn_send_http_redirect(cli, 302, index_files);
		return;
	}
	
	strcpy(local_path, document_root);
	memcpy(local_path + strlen(document_root), hm->path.p, hm->path.len);

	send_fd = open(local_path, O_RDONLY);
	if (send_fd < 0) {
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

	if (send_fd > 0) {
		char date[50] = "";
		char last_modified[50] = "";
		char etag[50] = "";
		time_t t = time(NULL);
		struct stat st;
		
		fstat(send_fd, &st);

		if (S_ISDIR(st.st_mode)) {
			close (send_fd);
			code = 404;
			goto end;
		}

		if (emn_http_is_not_modified(cli, &st)) {
			emn_send_http_error(cli, 304, NULL);
			goto end;
		}

		emn_time2gmt(date, sizeof(date), t);
		emn_time2gmt(last_modified, sizeof(last_modified), st.st_mtime);
		emh_http_construct_etag(etag, sizeof(etag), &st);
		
		emn_send_http_status_line(cli, code);
    	emn_printf(cli,
				"Date: %s\r\n"
				"Last-Modified: %s\r\n"
				"Etag: %s\r\n"
				"Content-Type: %s\r\n"
				"Content-Length: %zu\r\n"
				"Connection: %s\r\n"
				"\r\n", date, last_modified, etag, get_mime_type(&hm->path, "text/plain"),
					st.st_size, http_should_keep_alive(&hm->parser) ? "keep-alive" : "close");

		cli->send_fd = send_fd;
		if (hm->parser.method == HTTP_HEAD) {
			close(send_fd);
			cli->send_fd = -1;
		}
	}

	if (code != 200)
		emn_send_http_error(cli, code, NULL);
	
end:
	free(local_path);
}

static void serve_http(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;

	switch (hm->parser.method) {
	case HTTP_HEAD:
	case HTTP_GET:
	case HTTP_POST:
		send_http_file(cli, hm);
		break;
	default:
		emn_send_http_error(cli, 501, NULL);
	}

	if (!http_should_keep_alive(&hm->parser))
		cli->flags |= EMN_FLAGS_SEND_AND_CLOSE;
}

static int on_message_begin(http_parser *parser)
{
	int i;
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_names = hm->header_names;
	struct emn_str *header_values = hm->header_values;
	
	emn_str_init(&hm->url, NULL, 0);
	emn_str_init(&hm->body, NULL, 0);
	
	for (i = 0; i < hm->nheader; i++) {
		emn_str_init(header_names + i, NULL, 0);
		emn_str_init(header_values + i, NULL, 0);
	}
	
	hm->nheader = 0;
	ebuf_remove(&cli->rbuf, cli->rbuf.len);
	return 0;
}

static int on_url(http_parser *parser, const char *at, size_t len)
{	
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;

	if (hm->url.p)
		emn_str_increase(&hm->url, len);
	else
		emn_str_init(&hm->url, at, len);
	
    return 0;
}

static int on_header_field(http_parser *parser, const char *at, size_t len)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_names = hm->header_names;

	if (at[-1] == '\n') {
		if (hm->nheader == EMN_MAX_HTTP_HEADERS)
			return -1;
		(hm->nheader)++;
	}

	if (header_names[hm->nheader - 1].p)
		emn_str_increase(&header_names[hm->nheader - 1], len);
	else
		emn_str_init(&header_names[hm->nheader - 1], at, len);
	
    return 0;
}

static int on_header_value(http_parser *parser, const char *at, size_t len)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct emn_str *header_values = hm->header_values;
	
	if (header_values[hm->nheader - 1].p)
		emn_str_increase(&header_values[hm->nheader - 1], len);
	else {
		emn_str_init(&header_values[hm->nheader - 1], at, len);
	}

    return 0;
}

static int on_headers_complete(http_parser *parser)
{
	return 0;
}

static int on_body(http_parser *parser, const char *at, size_t len)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;

	if (hm->body.p)
		emn_str_increase(&hm->body, len);
	else
		emn_str_init(&hm->body, at, len);
	
    return 0;
}

static int on_message_complete(http_parser *parser)
{
	struct emn_client *cli = (struct emn_client *)parser->data;
	struct http_message *hm = (struct http_message *)cli->data;
	struct http_parser_url url;

	memset(&url, 0, sizeof(url));

	if (http_parser_parse_url(hm->url.p, hm->url.len, 0, &url)) {
		emn_log(LOG_ERR, "invalid url[%.*s]", (int)hm->url.len, hm->url.p);
		emn_send_http_error(cli, 400, NULL);
		return 0;
	} else {
		if (url.field_set & (1 << UF_PATH))
			emn_str_init(&hm->path, hm->url.p + url.field_data[UF_PATH].off, url.field_data[UF_PATH].len);

		if (url.field_set & (1 << UF_QUERY))
			emn_str_init(&hm->query, hm->url.p + url.field_data[UF_QUERY].off, url.field_data[UF_QUERY].len);
	}
		
	emn_call(cli, NULL, EMN_EV_HTTP_REQUEST, hm);

	return 0;
}

static http_parser_settings parser_settings = {
	.on_message_begin	 = on_message_begin,
	.on_url              = on_url,
	.on_header_field     = on_header_field,
	.on_header_value     = on_header_value,
	.on_headers_complete = on_headers_complete,
	.on_body             = on_body,
	.on_message_complete = on_message_complete
};

static void http_timeout_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	struct emn_client *cli = (struct emn_client *)w->data;
	emn_log(LOG_INFO, "http client(%p) timeout", cli);
	emn_client_destroy(cli);
}

static int emn_http_handler(struct emn_client *cli, int event, void *data)
{
	if (event == EMN_EV_ACCEPT) {
		struct http_message *hm = calloc(1, sizeof(struct http_message));
		http_parser_init(&hm->parser, HTTP_REQUEST);
		hm->parser.data = cli;
		cli->data = hm;

		ev_timer_init(&cli->timer, http_timeout_cb, EMN_HTTP_TIMEOUT, 0);
		cli->timer.data = cli;
		ev_timer_start(cli->srv->loop, &cli->timer);
	}

	if (!emn_call(cli, cli->handler, event, data) && event == EMN_EV_HTTP_REQUEST)
		serve_http(cli);

	if (event == EMN_EV_RECV) {
		size_t nparsed;
		struct ebuf *rbuf = (struct ebuf *)data;
		struct http_message *hm = (struct http_message *)cli->data;
		
		nparsed = http_parser_execute(&hm->parser, &parser_settings, rbuf->buf, rbuf->len);
		if (nparsed != rbuf->len) {
			emn_log(LOG_ERR, "%s", http_errno_description(HTTP_PARSER_ERRNO(&hm->parser)));
			emn_send_http_error(cli, 400, NULL);			
		} else {
			ebuf_append(&cli->rbuf, rbuf->buf, rbuf->len);
		}
	}
	
	return 0;
}

void emn_set_protocol_http(struct emn_server *srv, struct http_opts *opts)
{
	srv->proto_handler = emn_http_handler;
	srv->flags |= EMN_FLAGS_HTTP;
	srv->opts = opts;
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
	
	while (i < hm->nheader) {
		if (!emn_strvcasecmp(&header_names[i], name) && header_values[i].len > 0)
			return header_values + i;
		i++;
	}
	return NULL;
}

inline struct emn_str *emn_get_http_body(struct emn_client *cli)
{
	struct http_message *hm = (struct http_message *)cli->data;
	return &hm->body;
}

void emn_send_http_status_line(struct emn_client *cli, int code)
{
	const char *reason = get_http_status_message(code);
	emn_printf(cli, "HTTP/1.1 %d %s\r\nServer: Emn %s\r\n", code, reason, EMN_VERSION_STRING);
}

void emn_send_http_head(struct emn_client *cli, int code, ssize_t content_length, const char *extra_headers)
{
	emn_send_http_status_line(cli, code);
	
	if (content_length < 0)
		emn_printf(cli, "%s", "Transfer-Encoding: chunked\r\n");
	else
		emn_printf(cli, "Content-Length: %zd\r\n", content_length);

	if (extra_headers) 
		emn_send(cli, extra_headers, strlen(extra_headers));

	emn_send(cli, "\r\n", 2);
}

void emn_send_http_error(struct emn_client *cli, int code, const char *reason)
{
	struct http_message *hm = (struct http_message *)cli->data;
	
	if (!reason)
		reason = get_http_status_message(code);

	if (http_should_keep_alive(&hm->parser) && code < 400) {
		emn_send_http_head(cli, code, strlen(reason), "Content-Type: text/plain\r\nConnection: keep-alive\r\n");
	} else {
		emn_send_http_head(cli, code, strlen(reason), "Content-Type: text/plain\r\nConnection: close\r\n");
	}
	
	if (hm->parser.method != HTTP_HEAD)
		emn_send(cli, reason, strlen(reason));
}

void emn_send_http_redirect(struct emn_client *cli, int code, const char *location)
{
	char body[128] = "";
	struct http_message *hm = (struct http_message *)cli->data;
	
	snprintf(body, sizeof(body), "<p>Moved <a href=\"%s\">here</a></p>", location);  

	emn_send_http_status_line(cli, code);

	emn_printf(cli,
		"Location: %s\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %zu\r\n"
		"Cache-Control: no-cache\r\n", location, strlen(body));
	
	emn_send(cli, "\r\n", 2);

	if (hm->parser.method != HTTP_HEAD)
		emn_send(cli, body, strlen(body));
}

void emn_send_http_chunk(struct emn_client *cli, const char *buf, size_t len)
{
	emn_printf(cli, "%zX\r\n", len);
	emn_send(cli, buf, len);
	emn_send(cli, "\r\n", 2);
}

void emn_printf_http_chunk(struct emn_client *cli, const char *fmt, ...)
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
		emn_send_http_chunk(cli, str, len);
		free(str);
	}
}

