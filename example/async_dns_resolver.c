#include "emn.h"
#include <stdio.h>

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

static void dnscb(struct dns_ctx *ctx, struct dns_rr_a4 *result, void *data)
{
	int i = 0;
	struct in_addr *in = result->dnsa4_addr;

	printf("\n%s\n", (char *)data);
	
	for (; i< result->dnsa4_nrr; i++)
		printf("%s\n", inet_ntoa(in[i]));
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	
	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	if (emn_ares(loop, "www.baidu.com", dnscb) < 0)
		goto err;

	if (emn_ares(loop, "qq.com", dnscb) < 0)
		goto err;
	
	ev_run(loop, 0);

err:
	emn_ares_free();
	printf("exit...\n");

	return 0;
}

