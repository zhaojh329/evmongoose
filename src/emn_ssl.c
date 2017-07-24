#include "emn_ssl.h"
#include "emn_utils.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

static int print_err_cb(const char *str, size_t len, void *fp)
{
	syslog(LOG_ERR, "%s", str);
	return 0;
}

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type)
{
	SSL_CTX *ctx = NULL;

	SSL_library_init();

	/* creates a new SSL_CTX object */
	if (type == EMN_TYPE_SERVER)
		ctx = SSL_CTX_new(SSLv23_server_method());
	else
		ctx = SSL_CTX_new(SSLv23_client_method());

	if (!ctx) {
		emn_log(LOG_ERR, "Failed to create SSL context");
		return NULL;
	}

	/* loads the first certificate stored in file into ctx */
	if (!SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM)) {
		ERR_print_errors_cb(print_err_cb, NULL);
		goto err;
	}
		
	/* adds the first private RSA key found in file to ctx */
	if (!SSL_CTX_use_RSAPrivateKey_file(ctx, key, SSL_FILETYPE_PEM)) {
		ERR_print_errors_cb(print_err_cb, NULL);
		goto err;
	}
	
	return ctx;
	
err:
	SSL_CTX_free(ctx);
	return NULL;
}

