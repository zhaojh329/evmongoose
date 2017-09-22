#ifndef __EMN_HTTP_H_
#define __EMN_HTTP_H_

#include "emn.h"
#include <http_parser.h>

#define EMN_FLAGS_HTTP			(1 << 10)

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
 * Attaches a built-in HTTP event handler to the given emn_server.
 * The user-defined event handler will receive following extra events:
 *
 * - EMN_EV_HTTP_REPLY: The HTTP reply has arrived. Parsed HTTP request
 *  is passed as 'struct http_message' through the handler's 'void *data' pointer.
 */
void emn_cli_set_protocol_http(struct emn_client *cli);

/*
 * Searches and returns the header 'name' in parsed HTTP message 'hm'.
 * If header is not found, NULL is returned.
 * Example: struct emn_str *host = emn_get_http_header(hm, "Host");
 */
struct emn_str *emn_get_http_header(struct http_message *hm, const char *name);

/*
 * Sends the http response status line and automatically
 * sends one header: "Server: Emn: 0.1"
 *
 * Example:
 *      emn_send_http_status_line(cli, 200);
 *
 * Will output:
 *      HTTP/1.1 200 OK\r\n
 *      Server: Emn: 0.1\r\n
 */
void emn_send_http_status_line(struct emn_client *cli, int code);

/*
 * Sends the http response line and headers.
 * This function sends the http response line with the code, and automatically
 * sends one header: either "Content-Length" or "Transfer-Encoding".
 * If `content_length` is negative, then "Transfer-Encoding: chunked" header
 * is sent, otherwise, "Content-Length" header is sent.
 *
 * NOTE: If `Transfer-Encoding` is `chunked`, then message body must be sent
 * using `emn_send_http_chunk()` or `emn_printf_http_chunk()` functions.
 * Otherwise, `emn_send()` or `emn_printf()` must be used.
 * Extra headers could be set through `extra_headers`. Note `extra_headers`
 * must be terminated by a new line('\r\n').
 */
void emn_send_http_head(struct emn_client *cli, int code, ssize_t content_length, const char *extra_headers);

/*
 * Sends an http error response. If reason is NULL, the message will be inferred
 * from the error code (if supported).
 */
void emn_send_http_error(struct emn_client *cli, int code, const char *reason);

/*
 * Sends a http redirect response. `code` should be either 301 
 * or 302 and `location` point to the new location.
 *
 * Example: emn_send_http_redirect(cli, 302, "/login");
 */
void emn_send_http_redirect(struct emn_client *cli, int code, const char *location);

/*
 * Sends buffer `buf` of size `len` to the client using chunked HTTP encoding.
 * This function sends the buffer size as hex number + newline first, then
 * the buffer itself, then the newline.
 * For example, `emn_send_http_chunk(cli, "foo", 3)` whill output `3\r\nfoo\r\n`
 *
 * NOTE: The HTTP header "Transfer-Encoding: chunked" should be sent prior to using this function.
 *
 * NOTE: do not forget to send an empty chunk at the end of the response,
 * to tell the client that everything was sent.
 *
 * Example:
 *   emn_send_http_chunk(cli, "%s", "my response!");
 *   emn_send_http_chunk(cli, "", 0); // Tell the client we're finished
 */
int emn_send_http_chunk(struct emn_client *cli, const char *buf, size_t len);

/*
 * Sends a printf-formatted HTTP chunk.
 * Functionality is similar to `emn_send_http_chunk()`.
 */
int emn_printf_http_chunk(struct emn_client *cli, const char *fmt, ...);

#endif
