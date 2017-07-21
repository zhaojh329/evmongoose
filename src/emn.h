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

#define EMN_TCP_RECV_BUFFER_SIZE 1024

/* Events. Meaning of event parameter (evp) is given in the comment. */
#define EMN_EV_POLL		0   /* Sent to each connection on each mg_mgr_poll() call */
#define EMN_EV_ACCEPT	1  	/* New connection accepted. union socket_address * */
#define EMN_EV_CONNECT	2 	/* connect() succeeded or failed. int *  */
#define EMN_EV_RECV		3   /* Data has benn received. int *num_bytes */
#define EMN_EV_SEND		4   /* Data has been written to a socket. int *num_bytes */
#define EMN_EV_CLOSE	5   /* Connection is closed. NULL */
#define EMN_EV_TIMER	6   /* now >= conn->ev_timer_time. double * */

struct emn_object;
struct emn_server;
struct emn_client;

typedef void (*emn_event_handler_t)(struct emn_client *cli, int event, void *data);
								   
struct emn_server *emn_bind(struct ev_loop *loop, const char *address, emn_event_handler_t ev_handler);
void emn_server_destroy(struct emn_server *srv);
void emn_client_destroy(struct emn_client *cli);

void emn_set_protocol_http(struct emn_server *srv);

struct ebuf *emn_get_rbuf(struct emn_client *cli);
struct ebuf *emn_get_sbuf(struct emn_client *cli);

#endif
