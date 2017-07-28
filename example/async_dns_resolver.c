#include "emn.h"
#include <stdio.h>

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

static void ares_cb(struct emn_ares_query *eq, struct dns_rr_a4 *result)
{
	int i = 0;
	struct in_addr *in = result->dnsa4_addr;

	printf("\n%s\n", eq->name);
	
	for (; i< result->dnsa4_nrr; i++)
		printf("%s\n", inet_ntoa(in[i]));
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct emn_ares *ares;
	
	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	ares = emn_ares_init(loop, ares_cb);

	emn_ares_resolve(ares, "www.baidu.com", NULL);
	emn_ares_resolve(ares, "qq.com", NULL);
	
	ev_run(loop, 0);

	emn_ares_free(ares);

	printf("exit...\n");

	return 0;
}

