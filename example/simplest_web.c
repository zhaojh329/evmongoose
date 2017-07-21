#include <emn.h>
#include <stdio.h>

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

void event_handler(struct emn_object *obj, int event, void *data)
{
	
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct emn_server *srv = NULL;
	const char *address = "8000";
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	srv = emn_bind(loop, address, event_handler);
	if (!srv) {
		printf("emn_bind failed\n");
		goto err;
	}
	
	emn_set_protocol_http_websocket(srv);
	printf("listen on: %s\n", address);
	
	ev_run(loop, 0);

err:	
	emn_server_destroy(srv);
	printf("Server exit...\n");
		
	return 0;
}
