#ifndef __EMN_INTERNAL_H_
#define __EMN_INTERNAL_H_

#include "emn.h"
#include "list.h"

#if (EMN_USE_OPENSSL)
#include <openssl/ssl.h>
#elif (EMN_USE_CYASSL)
#include <wolfssl/ssl.h>
#endif

#define EMN_TYPE_SERVER		0
#define EMN_TYPE_CLIENT		1

struct emn_server {
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	ev_io ior;
	uint32_t flags;
	void *opts;		/* Pointing to protocol related structures */
#if (EMN_USE_OPENSSL)
	SSL_CTX *ssl_ctx;
#elif (EMN_USE_CYASSL)
	WOLFSSL_CTX *ssl_ctx;
#endif
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
	uint32_t flags;
	struct ebuf rbuf;	/* recv buf */
	struct ebuf sbuf;	/* send buf */
	void *data;			/* Pointing to protocol related structures */
	ev_timer timer;
#if (EMN_USE_OPENSSL)	
	SSL *ssl;
#elif (EMN_USE_CYASSL)
	WOLFSSL *ssl;
#endif
	struct emn_server *srv;
	struct list_head list;
};

int emn_call(struct emn_client *cli, emn_event_handler_t handler, int event, void *data);

#endif
