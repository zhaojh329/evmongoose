#include "emn_ssl.h"
#include "emn_utils.h"

#if (EMN_USE_OPENSSL)
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
		
	/*
	 * adds the first private RSA key found in file to ctx.
	 *
	 * checks the consistency of a private key with the corresponding 
	 * certificate loaded into ctx. If more than one key/certificate 
	 * pair (RSA/DSA) is installed, the last item installed will be checked.
	 */
	if (!SSL_CTX_use_RSAPrivateKey_file(ctx, key, SSL_FILETYPE_PEM)) {
		ERR_print_errors_cb(print_err_cb, NULL);
		goto err;
	}
	
	return ctx;
	
err:
	SSL_CTX_free(ctx);
	return NULL;
}
#elif (EMN_USE_CYASSL)
WOLFSSL_CTX *emn_ssl_init(const char *cert, const char *key, int type)
{
	WOLFSSL_CTX *ctx = NULL;

	/* Initialize wolfSSL */
	wolfSSL_Init();

	/* Create the WOLFSSL_CTX */
	if (type == EMN_TYPE_SERVER)
		ctx = wolfSSL_CTX_new(wolfSSLv23_server_method_ex(NULL));
	else
		ctx = wolfSSL_CTX_new(wolfSSLv23_client_method_ex(NULL));

	/* Load server certificates into WOLFSSL_CTX */
	if (wolfSSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
		emn_log(LOG_ERR, "Error loading certificate file");
		goto err;
	}

	/* Load keys */
	if (wolfSSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) != SSL_SUCCESS){
		emn_log(LOG_ERR, "Error loading key");
		goto err;
	}

	return ctx;
	
err:
	wolfSSL_CTX_free(ctx);
	wolfSSL_Cleanup();
	return NULL;
}
#endif
