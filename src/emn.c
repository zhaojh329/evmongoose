#include "emn.h"
#include "list.h"

struct emn_server {
	emn_event_handler_t handler;
	emn_event_handler_t proto_handler;
	
	int sock;
	ev_io ior;
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

        if (cli->proto_handler)
            cli->proto_handler(cli, EMN_EV_RECV, &len);
        else
            cli->handler(cli, EMN_EV_RECV, &len);
	} else if (len == 0) {
		/* Orderly shutdown of the socket, try flushing output. */
		cli->handler(cli, EMN_EV_CLOSE, NULL);
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
	list_add(&cli->list, &srv->client_list);
	
	ev_io_init(&cli->ior, ev_read_cb, sock, EV_READ);
	cli->ior.data = cli;
	ev_io_start(loop, &cli->ior);
	
	ev_io_init(&cli->iow, ev_write_cb, sock, EV_WRITE);
	cli->ior.data = cli;

    srv->handler(cli, EMN_EV_ACCEPT, &sin);
}

static void emn_http_handler(struct emn_client *cli, int event, void *data)
{
	cli->handler(cli, event, data);
}

void emn_set_protocol_http(struct emn_server *srv)
{
	srv->proto_handler = emn_http_handler;
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
		
		ebuf_free(&cli->rbuf);
		ebuf_free(&cli->sbuf);
		
		ev_io_stop(cli->srv->loop, &cli->ior);
		ev_io_stop(cli->srv->loop, &cli->iow);
		
		list_del(&cli->list);
		free(cli);
	}
}