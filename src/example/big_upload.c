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

static void handle_upload(struct mg_connection *nc, int ev, void *p)
{
	struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
	struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;

	switch (ev) {
	case MG_EV_HTTP_PART_BEGIN:
		data = calloc(1, sizeof(struct file_writer_data));
		data->fp = tmpfile();
		data->bytes_written = 0;

		if (data->fp == NULL) {
			mg_printf(nc, "%s",
				"HTTP/1.1 500 Failed to open a file\r\n"
				"Content-Length: 0\r\n\r\n");
			nc->flags |= MG_F_SEND_AND_CLOSE;
			return;
		}
		nc->user_data = (void *) data;

		printf("file name:%s\n", mp->file_name);
		printf("var name:%s\n", mp->var_name);
		break;
			
	case MG_EV_HTTP_PART_DATA:
		if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
			mg_printf(nc, "%s",
				"HTTP/1.1 500 Failed to write to a file\r\n"
				"Content-Length: 0\r\n\r\n");
			nc->flags |= MG_F_SEND_AND_CLOSE;
			return;
		}
		data->bytes_written += mp->data.len;
		break;
		
	case MG_EV_HTTP_PART_END:
		mg_printf(nc,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n"
			"Upload %s(%ld) to to a temp file\n\n",
			mp->file_name, ftell(data->fp));

		nc->flags |= MG_F_SEND_AND_CLOSE;
		fclose(data->fp);
		free(data);
		nc->user_data = NULL;
		break;
	}
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	switch (ev) {
	case MG_EV_HTTP_REQUEST:
		// Invoked when the full HTTP request is in the buffer (including body).
		handle_request(nc);
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

	mg_mgr_init(&mgr, NULL, loop);
	nc = mg_bind(&mgr, s_http_port, ev_handler);

	if (!nc) {
		printf("Bind failed\n");
	} else {
		mg_register_http_endpoint(nc, "/upload", handle_upload);
		// Set up HTTP server parameters
		mg_set_protocol_http_websocket(nc);

		printf("Starting web server on port %s\n", s_http_port);
		
		ev_run(loop, 0);
	}
	printf("Server exit...\n");
	
	mg_mgr_free(&mgr);

	return 0;
}

