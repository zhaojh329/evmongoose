#ifndef __EMN_UTILS_H_
#define __EMN_UTILS_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define emn_log(priority, format...) __emn_log(__FILENAME__, __LINE__, priority, format)

void  __emn_log(const char *filename, int line, int priority, const char *format, ...);


#if 0
 * Address format: [PROTO://][HOST]:PORT
 * Example: "8000" - Proto defaults to TCP
 * 		 	"tcp://*:8000"
 * 			"udp://*:8000"
 * 		 	"tcp://192.168.1.1:8000"
 * 		 	"udp://192.168.1.1:8000"
 *
 * Return:
 *   -1   on parse error
 *    0   parse ok
 #endif
int emn_parse_address(const char *address, struct sockaddr_in *sin, int *proto);

 
int emn_open_listening_socket(struct sockaddr_in *sin, int type, int proto);

/* Gets the time of the current microsecond precision */
double emn_time();

/*
 * Sun, 23 Jul 2017 08:19:18 GMT
 * Converts the time of type time_t to a time string in GMT format
 */
void emn_time2gmt(char *buf, size_t buf_len, time_t t);

/*
 * Sun, 23 Jul 2017 08:19:18 GMT
 * Converts a GMT formatted time string to the time_t type
 */
time_t emn_gmt2time(const char *datetime);

#endif
