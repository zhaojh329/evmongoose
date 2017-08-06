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


int xx = 0;
static void ev_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	ares_channel channel = (ares_channel)w->data;
	if (xx++ == 10)
		exit(0);
	printf("ev_read_cb\n");
	ares_process_fd(channel, w->fd, -1);
}

ev_io ior1;
ev_io ior2;

void emn_ares_callback(void *arg, int status, int timeouts, unsigned char *abuf, int alen)
{
	struct hostent *host = NULL;
	if (status == ARES_SUCCESS) {
		char **p;
		status = ares_parse_a_reply(abuf, alen, &host, NULL, NULL);

		for (p = host->h_addr_list; *p; p++) {
			char addr_buf[46] = "??";
			ares_inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
			printf("%-32s%s", host->h_name, addr_buf);
			puts("");
		}
	}
	printf("status = %d\n", status);
}

int ss = 0;
int ares_sock_callback(ares_socket_t socket_fd, int type, void *userdata)
{
	ares_channel channel = (ares_channel)userdata;
	printf("socket_fd = %d, type = %d\n", socket_fd, type);

	if (ss == 0) {
		ss = 1;
		ev_io_init(&ior1, ev_read_cb, socket_fd, EV_READ);
		ior1.data = channel;
		ev_io_start(EV_DEFAULT, &ior1);
 	} else {
		ev_io_init(&ior2, ev_read_cb, socket_fd, EV_READ);
		ior2.data = channel;
		ev_io_start(EV_DEFAULT, &ior2);
	}
	
	return ARES_SUCCESS;
}

void resolve_handler(struct hostent *host, void *data)
{
	char **p;

	for (p = host->h_addr_list; *p; p++) {
		char addr_buf[46] = "??";
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
		
	ev_run(loop, 0);

	printf("exit...\n");

	return 0;
}

