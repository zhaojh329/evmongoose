#ifndef __EMN_UTILS_H_
#define __EMN_UTILS_H_

#include <netinet/in.h>

int parse_address(const char *str, struct sockaddr_in *sin, int *proto);
int open_listening_socket(struct sockaddr_in *sin, int type, int proto);

#endif
