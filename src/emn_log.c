#include "emn_log.h"

void emn_log_init(const char *ident, int opt)
{
	if (opt | EMN_LOG_ON) {
		openlog(const char *ident, int option, int facility);
	}
}

