#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mongoose.h>

static const char *s_http_port = "8000";

struct file_writer_data {
	FILE *fp;
	size_t bytes_written;
};

static void handle_request(struct mg_connection *nc)
{
	// This handler gets for all endpoints but /upload
	mg_printf(nc, "%s",
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<html><body>Upload example."
		"<form method=\"POST\" action=\"/upload\" "
		"  enctype=\"multipart/form-data\">"
		"<input type=\"file\" name=\"file\" /> <br/>"
		"<input type=\"submit\" value=\"Upload\" />"
		"</form></body></html>");
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

static struct mg_str http_upload_fname(struct mg_connection *nc, struct mg_str fname)
{
	//return mg_mk_str(strdup("/tmp/xxx.txt")); /* Change path */
	return fname;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	switch (ev) {
	case MG_EV_HTTP_REQUEST:
		// Invoked when the full HTTP request is in the buffer (including body).
		handle_request(nc);
		break;
	case MG_EV_HTTP_PART_BEGIN:
	case MG_EV_HTTP_PART_DATA:
	case MG_EV_HTTP_PART_END:
		mg_file_upload_handler(nc, ev, ev_data, http_upload_fname);
		break;
	case MG_EV_HTTP_MULTIPART_REQUEST_END:
		// TODO: ?
		
		break;
	}
}

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
	printf("Got signal: %d\n", w->signum);
	ev_break(loop, EVBREAK_ALL);
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	ev_signal sig_watcher;
	struct mg_mgr mgr;
	struct mg_connection *nc;

	ev_signal_init(&sig_watcher, signal_cb, SIGINT);
	ev_signal_start(loop, &sig_watcher);

	mg_mgr_init(&mgr, NULL);
	nc = mg_bind(&mgr, s_http_port, ev_handler);

	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);

	printf("Starting web server on port %s\n", s_http_port);
	
	ev_run(loop, 0);

	printf("Server exit...\n");
	
	mg_mgr_free(&mgr);

	return 0;
}

