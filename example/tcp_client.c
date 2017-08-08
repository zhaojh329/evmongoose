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
	case EMN_EV_RECV: {
			struct ebuf *eb = (struct ebuf *)data;
			printf("recv: %.*s\n", (int)eb->len, eb->buf);

			emn_printf(cli, "%f: I'm emn", emn_time());
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
	const char *address = "192.168.0.101:8080";
	
	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	cli = emn_connect(loop, address, event_handler);
	if (!cli) {
		printf("emn_connect failed\n");
		return -1;
	}
	
	ev_run(loop, 0);

	printf("exit...\n");

	return 0;
}

