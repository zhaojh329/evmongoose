#include "emn.h"
#include <stdio.h>

#if 1
static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

int event_handler(struct emn_client *cli, int event, void *data)
{
	switch (event) {
    case EMN_EV_ACCEPT: {
			struct sockaddr_in *sin = (struct sockaddr_in *)data;
	        printf("new conn from %s:%d\n", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
        	break;
    	}
	case EMN_EV_RECV: {
#if 0
			struct ebuf *rbuf = emn_get_rbuf(cli);
			int len = *(int *)data;
			printf("recv %d: [%.*s]\n", len, (int)rbuf->len, rbuf->buf);
			//ebuf_remove(rbuf, len);
#endif			
			break;
		}
	case EMN_EV_HTTP_REQUEST: {
#if 0
			enum http_method method = emn_get_http_method(cli);
			struct emn_str *url = emn_get_http_url(cli);
			struct emn_str *path = emn_get_http_path(cli);
			struct emn_str *query = emn_get_http_query(cli);
			struct emn_str *host = emn_get_http_header(cli, "host");
			struct emn_str *body = emn_get_http_body(cli);
			
			printf("method: %s\n", http_method_str(method));
			printf("url: %.*s\n", (int)url->len, url->p);
			printf("path: %.*s\n", (int)path->len, path->p);
			printf("query: %.*s\n", (int)query->len, query->p);
			printf("proto: %d.%d\n", emn_get_http_version_major(cli), emn_get_http_version_minor(cli));
			printf("Host: %.*s\n", (int)host->len, host->p);
			printf("body: %.*s\n", (int)body->len, body->p);
#endif
			break;
		}
	case EMN_EV_CLOSE: {
			printf("client(%p) closed\n", cli);
			break;
		}
    default:
		break;
    }

	return 0;
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct emn_server *srv = NULL;
	const char *address = "8000";

	openlog(NULL, LOG_PERROR | LOG_PID, 0);
	
	printf("emn version: %d.%d\n", EMN_VERSION_MAJOR, EMN_VERSION_MINOR);
	
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	srv = emn_bind(loop, address, event_handler);
	if (!srv) {
		printf("emn_bind failed\n");
		goto err;
	}
	
	emn_set_protocol_http(srv, NULL);
	printf("listen on: %s\n", address);
	
	ev_run(loop, 0);

err:	
	emn_server_destroy(srv);
	printf("Server exit...\n");

	return 0;
}

#else
int main(int argc, char **argv)
{

	struct emn_str str1;

	emn_str_init(&str1, "AB12", 4);

	printf("%d\n", emn_strvcasecmp(&str1, "aB12"));
}
#endif

