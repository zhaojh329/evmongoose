#include <mongoose.h>

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
	struct mg_connection *nc;
	struct mg_mqtt_broker brk;
	const char *address = "0.0.0.0:1882";
	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);
	
	mg_mgr_init(&mgr, NULL, loop);
	mg_mqtt_broker_init(&brk, NULL);
	
	if ((nc = mg_bind(&mgr, address, mg_mqtt_broker)) == NULL) {
		fprintf(stderr, "mg_bind(%s) failed\n", address);
		goto err;
	}
	nc->user_data = &brk;

	printf("MQTT broker started on %s\n", address);

	/*
	* TODO: Add a HTTP status page that shows current sessions
	* and subscriptions
	*/

	
	/*
	* Iterates over all MQTT session connections. Example:
	*
	* ```c
	* struct mg_mqtt_session *s;
	* for (s = mg_mqtt_next(brk, NULL); s != NULL; s = mg_mqtt_next(brk, s)) {
	*   // Do something
	* }
	* ```
	*/
	
	ev_run(loop, 0);

err:
	printf("exit...\n");
	
	mg_mgr_free(&mgr);

	return 0;
}

