#include <mongoose.h>

static void ev_handler(struct mg_connection *nc, int ev, void *p)
{
	struct mbuf *io = &nc->recv_mbuf;

	switch (ev) {
	case MG_EV_RECV:
		mg_send(nc, io->buf, io->len);  // Echo message back
		mbuf_remove(io, io->len);       // Discard message from recv buffer
		break;
	default:
		break;
	}
}

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct mg_mgr mgr;
	const char *server1 = "7000";
	const char *server2 = "8000";

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);

	if  (!mg_bind(&mgr, server1, ev_handler))
		printf("Failed to open listener:%s\n", server1);
	else
		printf("Starting tcp server on port %s\n", server1);

	if (!mg_bind(&mgr, server2, ev_handler))
		printf("Failed to open listener:%s\n", server2);
	else
		printf("Starting tcp server on port %s\n", server2);

	ev_run(loop, 0);

	printf("Server exit...\n");
	
	mg_mgr_free(&mgr);
		
	return 0;
}

