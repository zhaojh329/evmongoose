#include <mongoose.h>

static int s_show_headers = 0;
static const char *s_show_headers_opt = "--show-headers";

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;

	switch (ev) {
	case MG_EV_CONNECT:
		if (*(int *) ev_data != 0)
			fprintf(stderr, "connect() failed: %s\n", strerror(*(int *) ev_data));
		break;
	case MG_EV_HTTP_REPLY:
		nc->flags |= MG_F_CLOSE_IMMEDIATELY;
		if (s_show_headers) {
			fwrite(hm->message.p, 1, hm->message.len, stdout);
		} else {
			fwrite(hm->body.p, 1, hm->body.len, stdout);
		}
		putchar('\n');
		break;
	case MG_EV_CLOSE:
			printf("Server closed connection\n");
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

int main(int argc, char *argv[])
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct mg_mgr mgr;
	int i;

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);

	/* Process command line arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], s_show_headers_opt) == 0) {
			s_show_headers = 1;
		} else if (strcmp(argv[i], "--hexdump") == 0 && i + 1 < argc) {
			mgr.hexdump_file = argv[++i];
		} else {
			break;
		}
	}

	if (i + 1 != argc) {
		fprintf(stderr, "Usage: %s [%s] [--hexdump <file>] <URL>\n", argv[0],
		s_show_headers_opt);
		exit(EXIT_FAILURE);
	}

	mg_connect_http(&mgr, ev_handler, argv[i], NULL, NULL);

	ev_run(loop, 0);

	printf("exit...\n");
	
	mg_mgr_free(&mgr);

	return 0;
}

