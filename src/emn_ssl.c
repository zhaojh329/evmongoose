#include "emn_ssl.h"
#include "emn_utils.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

static int print_err_cb(const char *str, size_t len, void *fp)
{
	syslog(LOG_ERR, "%s", str);
	return 0;
}

int emn_ssl_init(void *obj, const char *cert, const char *key, int type)
{
	SSL_CTX *ctx = NULL;

	SSL_library_init();
	
	if (type == EMN_TYPE_SERVER)
		ctx = SSL_CTX_new(SSLv23_server_method());
	else
		ctx = SSL_CTX_new(SSLv23_client_method());

	if (!ctx) {
		emn_log(LOG_ERR, "Failed to create SSL context");
		return -1;
	}

	if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) == 0) {
		ERR_print_errors_cb(print_err_cb, NULL);
		goto err;
	}
		

	if (SSL_CTX_use_RSAPrivateKey_file(ctx, key, SSL_FILETYPE_PEM) == 0) {
		ERR_print_errors_cb(print_err_cb, NULL);
		goto err;
	}
	
	return 0;
	
err:
	SSL_CTX_free(ctx);
	return -1;
}

