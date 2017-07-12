#include <mongoose.h>

static void dns_resolve_cb(struct mg_dns_message *msg, void *data, enum mg_resolve_err e)
{
	struct in_addr ina;
	struct mg_dns_resource_record *rr = NULL;
	
	if (!msg) {
		printf("resolve failed:");
		switch (e) {
		case MG_RESOLVE_NO_ANSWERS:
			printf(" No answers\n");
			break;
		case MG_RESOLVE_EXCEEDED_RETRY_COUNT:
			printf(" Exceeded retry count\n");
			break;
		case MG_RESOLVE_TIMEOUT:
			printf(" Timeout\n");
			break;
		default:
			printf(" Unknown error\n");
			break;
		}
		return;
	}
	
	while (1) {
		rr = mg_dns_next_record(msg, MG_DNS_A_RECORD, rr);
		if (!rr)
			break;

		if (mg_dns_parse_record_data(msg, rr, &ina, sizeof(ina)))
			break;

		printf("resolved(%s):%s\n", (char *)data, inet_ntoa(ina));
	}
	
	ev_break(EV_DEFAULT, EVBREAK_ALL);
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

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);

	/* Process command line arguments */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
		exit(0);
	}

	mg_resolve_async(&mgr, argv[1], MG_DNS_A_RECORD, dns_resolve_cb, argv[1]);

	ev_run(loop, 0);

	printf("exit...\n");
	
	mg_mgr_free(&mgr);

	return 0;
}


