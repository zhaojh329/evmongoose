#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "emn_utils.h"

/*
 * Address format: [PROTO://][HOST]:PORT
 *
 * HOST could be IPv4/IPv6 address or a host name.
 * `host` is a destination buffer to hold parsed HOST part.
 * `proto` is a returned socket type, either SOCK_STREAM or SOCK_DGRAM
 *
 * Return:
 *   -1   on parse error
 *    0   if HOST needs DNS lookup
 *   >0   length of the address string
 */
int parse_address(const char *str, struct sockaddr_in *sin, int *proto)
{
	unsigned int a, b, c, d, port = 0;
	int len = 0;

	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;

	*proto = SOCK_STREAM;

	if (strncmp(str, "udp://", 6) == 0) {
		str += 6;
		*proto = SOCK_DGRAM;
	} else if (strncmp(str, "tcp://", 6) == 0) {
		str += 6;
	}
	
	if (sscanf(str, "%u.%u.%u.%u:%u%n", &a, &b, &c, &d, &port, &len) == 5) {
		/* Bind to a specific IPv4 address, e.g. 192.168.1.1:8080 */
		sin->sin_addr.s_addr = htonl(((uint32_t)a << 24) | ((uint32_t)b << 16) | c << 8 | d);
		sin->sin_port = htons((uint16_t)port);
	} else if (sscanf(str, "%u%n", &port, &len) == 1) {
		/* If only port is specified, bind to IPv4, INADDR_ANY */
		sin->sin_port = htons((uint16_t)port);
	} else {
		return -1;
	}
	
	return len;
}

int open_listening_socket(struct sockaddr_in *sin, int type, int proto)
{
	int sock = -1;
	int on = 1;

	sock = socket(sin->sin_family, type | SOCK_NONBLOCK | SOCK_CLOEXEC, proto);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr *)sin, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		close(sock);
		return -1;
	}

	if (type == SOCK_STREAM && listen(sock, SOMAXCONN) < 0) {
		perror("listen");
		close(sock);
		return -1;
	}
	
	return sock;
}

