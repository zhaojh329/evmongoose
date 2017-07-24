#ifndef __EMN_HTTP_H_
#define __EMN_HTTP_H_

#include "emn.h"
#include <http_parser.h>

#define EMN_FLAGS_HTTP			(1 << 0)

#define EMN_MAX_HTTP_HEADERS	40

#define EMN_HTTP_TIMEOUT		30

/* HTTP and websocket events. void *ev_data is described in a comment. */
#define EMN_EV_HTTP_REQUEST	100	/* struct http_message * */
#define EMN_EV_HTTP_REPLY	101	/* struct http_message * */
#define EMN_EV_HTTP_CHUNK	102	/* struct http_message * */

struct http_opts {
	/* Path to web root directory */
	const char *document_root;

	/* List of index files. Default is "" */
	const char *index_files;
};

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

struct emn_server;
struct emn_client;

/*
 * Attaches a built-in HTTP event handler to the given emn_server.
 * The user-defined event handler will receive following extra events:
 *
 * - EMN_EV_HTTP_REQUEST: HTTP request has arrived. Parsed HTTP request
 *  is passed as 'struct http_message' through the handler's 'void *data' pointer.
 */
void emn_set_protocol_http(struct emn_server *srv, struct http_opts *opts);


/*
 * Searches and returns the header 'name' in parsed HTTP message 'hm'.
 * If header is not found, NULL is returned.
 * Example: struct emn_str *host = emn_get_http_header(hm, "Host");
 */
struct emn_str *emn_get_http_header(struct http_message *hm, const char *name);

void emn_send_http_status_line(struct emn_client *cli, int code);
void emn_send_http_head(struct emn_client *cli, int code, ssize_t content_length, const char *extra_headers);
void emn_send_http_error(struct emn_client *cli, int code, const char *reason);
void emn_send_http_redirect(struct emn_client *cli, int code, const char *location);
int emn_send_http_chunk(struct emn_client *cli, const char *buf, size_t len);
int emn_printf_http_chunk(struct emn_client *cli, const char *fmt, ...);

#endif
