#include "emn_ssl.h"
#include "emn_utils.h"

int emn_ssl_init(void *obj, const char *ssl_cert, int type)
{
	SSL_CTX *ctx = NULL;

	if (type == EMN_TYPE_SERVER)
		ctx = SSL_CTX_new(SSLv23_server_method());
	else
		ctx = SSL_CTX_new(SSLv23_client_method());

	if (!ctx) {
		emn_log(LOG_ERR, "Failed to create SSL context");
		return -1;
	}
	
	return 0;
}

