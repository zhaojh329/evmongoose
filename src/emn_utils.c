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

