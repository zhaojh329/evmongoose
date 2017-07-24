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

double emn_time()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
		emn_log(LOG_ERR, "gettimeofday:%s", strerror(errno));
		return 0;
	}
	return (double)tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
}

void emn_time2gmt(char *buf, size_t buf_len, time_t t)
{
	strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
}

time_t emn_gmt2time(const char *datetime)
{
	struct tm tm;
	strptime(datetime, "%a, %d %b %Y %H:%M:%S GMT", &tm);

	return mktime(&tm);
}

