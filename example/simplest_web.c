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
#if 1
			struct http_message *hm = (struct http_message *)data;
			struct emn_str *host = emn_get_http_header(hm, "Host");
			
			printf("method: %s\n", http_method_str(hm->parser.method));
			printf("url: %.*s\n", (int)hm->url.len, hm->url.p);
			printf("path: %.*s\n", (int)hm->path.len, hm->path.p);
			printf("query: %.*s\n", (int)hm->query.len, hm->query.p);
			printf("proto: %d.%d\n", hm->parser.http_major, hm->parser.http_minor);

			if (host)
				printf("Host: %.*s\n", (int)host->len, host->p);
			printf("body: %.*s\n", (int)hm->body.len, hm->body.p);

			emn_send_http_head(cli, 200, -1, NULL);
			emn_send_http_chunk(cli, "12", 2);
			emn_send_http_chunk(cli, "345", 3);
			emn_printf_http_chunk(cli, "Emn:%d\n", 123);
			emn_send_http_chunk(cli, NULL, 0);
			return 1;
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

	//signal(SIGPIPE, SIG_IGN);
	
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
	char *p;
	char *path = "a.html";


	p = rindex(path, '.');
	if (!p || !*p)
		return 1;

	printf("%c %zd\n", *(p + 1), strlen(path) + path - p - 1);
	return 0;

}
#endif

