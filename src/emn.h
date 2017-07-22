#ifndef __EMN_H_
#define __EMN_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <http_parser.h>
#include <ev.h>

#include "emn_config.h"
#include "emn_buf.h"
#include "emn_utils.h"

#define EMN_TCP_RECV_BUFFER_SIZE	1024
#define EMN_MAX_HTTP_HEADERS		40

/* Events. Meaning of event parameter (evp) is given in the comment. */
#define EMN_EV_POLL		0   /* Sent to each connection on each mg_mgr_poll() call */
#define EMN_EV_ACCEPT	1  	/* New connection accepted. union socket_address * */
#define EMN_EV_CONNECT	2 	/* connect() succeeded or failed. int *  */
#define EMN_EV_RECV		3   /* Data has benn received. int *num_bytes */
#define EMN_EV_SEND		4   /* Data has been written to a socket. int *num_bytes */
#define EMN_EV_CLOSE	5   /* Connection is closed. NULL */
#define EMN_EV_TIMER	6   /* now >= conn->ev_timer_time. double * */

/* HTTP and websocket events. void *ev_data is described in a comment. */
#define EMN_EV_HTTP_REQUEST	100	/* struct http_message * */
#define EMN_EV_HTTP_REPLY	101	/* struct http_message * */
#define EMN_EV_HTTP_CHUNK	102	/* struct http_message * */


#define EMN_FLAGS_HTTP	(1 << 0)

/* Flags that are settable by user */
#define EMN_FLAGS_SEND_AND_CLOSE	(1 << 1)	/* Push remaining data and close  */
#define EMN_FLAGS_CLOSE_IMMEDIATELY (1 << 2)	/* Disconnect */

struct emn_server;
struct emn_client;

/* Describes chunk of memory */
struct emn_str {
	const char *p; /* Memory chunk pointer */
	size_t len;    /* Memory chunk length */
};

struct http_opts {
	/* Path to web root directory */
	const char *document_root;

	/* List of index files. Default is "" */
	const char *index_files;
};

void emn_str_init(struct emn_str *str, const char *at, size_t len);

typedef int (*emn_event_handler_t)(struct emn_client *cli, int event, void *data);
								   
struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler);
void emn_server_destroy(struct emn_server *srv);
void emn_client_destroy(struct emn_client *cli);

void emn_set_protocol_http(struct emn_server *srv, struct http_opts *opts);

struct ebuf *emn_get_rbuf(struct emn_client *cli);
struct ebuf *emn_get_sbuf(struct emn_client *cli);

enum http_method emn_get_http_method(struct emn_client *cli);
struct emn_str *emn_get_http_url(struct emn_client *cli);
struct emn_str *emn_get_http_path(struct emn_client *cli);
struct emn_str *emn_get_http_query(struct emn_client *cli);
uint8_t emn_get_http_version_major(struct emn_client *cli);
uint8_t emn_get_http_version_minor(struct emn_client *cli);
struct emn_str *emn_get_http_header(struct emn_client *cli, const char *name);
struct emn_str *emn_get_http_body(struct emn_client *cli);

#endif
