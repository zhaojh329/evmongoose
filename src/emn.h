#ifndef __EMN_H_
#define __EMN_H_

#include <fcntl.h>
#include <ev.h>

#include "emn_config.h"
#include "emn_str.h"
#include "emn_buf.h"
#include "emn_utils.h"
#include "emn_http.h"
#include "emn_ares.h"

#define EMN_RECV_BUFFER_SIZE	1024

/* Events. Meaning of event parameter (evp) is given in the comment. */
#define EMN_EV_POLL		0   /* Sent to each connection on each mg_mgr_poll() call */
#define EMN_EV_ACCEPT	1  	/* New connection accepted. union socket_address * */
#define EMN_EV_CONNECT	2 	/* connect() succeeded or failed. int *  */
#define EMN_EV_RECV		3   /* Data has benn received. struct ebuf * */
#define EMN_EV_SEND		4   /* Data has been written to a socket. int *num_bytes */
#define EMN_EV_CLOSE	5   /* Connection is closed. NULL */

#define EMN_FLAGS_SEND_AND_CLOSE	(1 << 0)	/* Send remaining data and close  */
#define EMN_FLAGS_CLOSE_IMMEDIATELY (1 << 1)	/* Disconnect */
#define EMN_FLAGS_SSL				(1 << 2)	/* SSL is enabled on the server or client */
#define EMN_FLAGS_UDP				(1 << 3)	/* The server or client using the UDP protocol */

struct emn_server;
struct emn_client;

struct emn_bind_opts {
#if (EMN_SSL_ENABLED)
	/*
	 * SSL settings.
	 *
	 * Server certificate to present to clients or client certificate to
	 * present to tunnel dispatcher (for tunneled connections).
	 */
	const char *ssl_cert;

	/*
	 * Private key corresponding to the certificate. If ssl_cert is set but
	 * ssl_key is not, ssl_cert is used.
	 */
	const char *ssl_key;
#endif
};

typedef int (*emn_event_handler_t)(struct emn_client *cli, int event, void *data);

/*
 * Creates a server.
 * See `emn_bind_opt` for full documentation.
 */
struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler);

/*
 * Creates a server.
 *
 * The 'address' parameter specifies which address to bind to. 'HOST' part is optional.
 * 'address' can be just a port number, e.g. :8000. To bind to a specific
 * interface, an IP address can be specified, e.g. '1.2.3.4:8000'. By default,
 * a TCP server is created. To create UDP server, prepend 'udp://' prefix, 
 * e.g. 'udp://:8000'. To summarize, 'address' paramer has following
 * format: '[PROTO://][IP_ADDRESS]:PORT', where 'PROTO' could be 'tcp' or 'udp'.
 *
 * Returns a new emn_server or NULL on error.
 */
struct emn_server *emn_bind_opt(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler, 
								struct emn_bind_opts *opts);

/* De-initialises emn_server and release all resources associated with it */
void emn_server_destroy(struct emn_server *srv);

/* De-initialises emn_client and release all resources associated with it */
void emn_client_destroy(struct emn_client *cli);

/*
 * Sends data to the client.
 *
 * Note that sending functions do not actually push data to the socket.
 * They just append data to the output buffer. EMN_EV_SEND will be delivered when
 * the data has actually been pushed out.
 */
size_t emn_send(struct emn_client *cli, const void *buf, int len);

/*
 * Sends 'printf' style formatted data to the client.
 * See 'emn_send' for more details on send semantics.
 */
int emn_printf(struct emn_client *cli, const char *fmt, ...);

/* Get receive buf of the client */
struct ebuf *emn_get_rbuf(struct emn_client *cli);

/* Get send buf of the client */
struct ebuf *emn_get_sbuf(struct emn_client *cli);

void emn_client_set_flags(struct emn_client *cli, uint16_t flags);

#endif
