#include "emn.h"
#include <stdio.h>

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

int event_handler(struct emn_client *cli, int event, void *data)
{
	switch (event) {
	case EMN_EV_CONNECT: {
			int err = *(int *)data;
			printf("connect:%p %s\n", cli, strerror(err));
			break;
		}

	case EMN_EV_HTTP_REPLY: {
			struct http_message *hm = (struct http_message *)data;
			struct emn_str *server = emn_get_http_header(hm, "Server");
			
			printf("proto: %d.%d\n", hm->parser.http_major, hm->parser.http_minor);

			if (server)
				printf("server: %.*s\n", (int)server->len, server->p);
			
			printf("body: \n%.*s\n", (int)hm->body.len, hm->body.p);

			break;
		}
	case EMN_EV_CLOSE:
		printf("%p closed\n", cli);
		break;
    default:
		break;
    }

	return 0;
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct emn_client *cli = NULL;
	const char *address = "http://192.168.3.33/xxx?x=1";
	
	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	cli = emn_connect_http(loop, address, event_handler);
	if (!cli) {
		printf("emn_connect failed\n");
		return -1;
	}
	
	ev_run(loop, 0);

	printf("exit...\n");

	return 0;
}


