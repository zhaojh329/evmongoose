#include "emn.h"
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <ares.h>
#include <stdio.h>
#include <string.h>
#include <arpa/nameser.h>

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

void resolve_handler(int status, struct hostent *host, void *data)
{
	char **p;

	if (status == EMN_ARES_TIMEOUT) {
		printf("resolve timeout\n");
		return;
	}

	for (p = host->h_addr_list; *p; p++) {
		char addr_buf[46] = "";
		ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
		printf("%-32s%s", host->h_name, addr_buf);
		puts("");
	}
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	const char *name[] = {"www.baidu.com", "qq.com", NULL};
	
	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	emn_resolve(loop, name, resolve_handler, NULL);
	emn_resolve_single(loop, "sina.com", resolve_handler, NULL);
	
	ev_run(loop, 0);

	printf("exit...\n");

	return 0;
}

