#include "emn_utils.h"

void __emn_log(const char *filename, int line, int priority, const char *format, ...)
{
	va_list ap;
	static char buf[128];

	snprintf(buf, sizeof(buf), "(%s:%d) ", filename, line);
	
	va_start(ap, format);
	vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), format, ap);
	va_end(ap);

	syslog(priority, "%s", buf);
}

int emn_parse_address(const char *address, struct sockaddr_in *sin, int *proto)
{
	const char *str;
	char *p;
	char host[16] = "";
	uint16_t port = 0;

	assert(address);
	
	memset(sin, 0, sizeof(struct sockaddr_in));
	
	sin->sin_family = AF_INET;
	*proto = SOCK_STREAM;
	str = address;
	
	if (strncmp(str, "udp://", 6) == 0) {
		str += 6;
		*proto = SOCK_DGRAM;
	} else if (strncmp(str, "tcp://", 6) == 0) {
		str += 6;
	}

	p = strchr(str, ':');
	if (p) {
		if (p - str > 15)
			return -1;
		
		memcpy(host, str, p - str);

		if (strcmp(host, "*")) {	
			if (inet_pton(AF_INET, host, &sin->sin_addr) <= 0)
				return -1;
		}
		str = p + 1;
	}

	port = atoi(str);
	if (port <= 0)
		return -1;

	sin->sin_port = htons(port);
	
	return 0;
}


int emn_open_listening_socket(struct sockaddr_in *sin, int type, int proto)
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

double emn_time()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		emn_log(LOG_ERR, "gettimeofday:%s", strerror(errno));
		return 0;
	}
	return (double)tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
}

