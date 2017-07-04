#include <mongoose.h>

static void ev_handler(struct mg_connection *nc, int ev, void *p)
{
	struct mbuf *io = &nc->recv_mbuf;

	switch (ev) {
	case MG_EV_RECV:
		printf("recv:%s\n", io->buf);
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

static void stdin_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	int len = 0;
	char buf[1024] = "";
	struct mg_connection *nc = (struct mg_connection *)w->data;
	
	len = read(w->fd, buf, sizeof(buf));
	
	if (len > 0) {
		mg_send(nc, buf, len);
	}
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	ev_io stdin_watcher;
	struct mg_mgr mgr;
	struct mg_connection *nc;

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);
	
	nc = mg_connect(&mgr, "7000", ev_handler);
	if (!nc) {
		printf("connect failed\n");
		goto err;
	}
	
	ev_io_init(&stdin_watcher, stdin_read_cb, 0, EV_READ);
	stdin_watcher.data = nc;
	ev_io_start(loop, &stdin_watcher);
	
	ev_run(loop, 0);
	
err:
	printf("Server exit...\n");
	
	mg_mgr_free(&mgr);
		
	return 0;
}

