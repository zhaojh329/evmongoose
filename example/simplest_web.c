#include <mongoose.h>

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	char buf[] = "Hello EvMG: API Test\n";
	struct http_message *hm = (struct http_message *)ev_data;
	
	switch (ev) {
	case MG_EV_HTTP_REQUEST:
		if (!mg_vcmp(&hm->uri, "/test")) {
			printf("Request test...\n");
			mg_printf(nc, "HTTP/1.1 200 OK\r\n"
						  "Content-Length: %d\r\n\r\n"
						  "%s", (int)strlen(buf), buf);
		} else {
			mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
		}
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
	struct mg_connection *nc;

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);
	
	printf("Starting web server on port %s\n", s_http_port);
	nc = mg_bind(&mgr, s_http_port, ev_handler);
	if (nc == NULL) {
		printf("Failed to create listener\n");
		return 1;
	}

	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = ".";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";

	ev_run(loop, 0);

	printf("Server exit...\n");
	
	mg_mgr_free(&mgr);
		
	return 0;
}
