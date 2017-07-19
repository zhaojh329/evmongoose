#ifndef __EMN_H_
#define __EMN_H_

#define EMN_VERSION_MAJOR 0
#define EMN_VERSION_MINOR 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <http_parser.h>
#include <ev.h>

struct emn_server;
struct emn_client;

typedef void (*emn_event_handler_t)(void *obj, int event, void *data);
								   
struct emn_server *emn_bind(const char *address, emn_event_handler_t event_handler);
void emn_server_destroy(struct emn_server *srv);

#endif
