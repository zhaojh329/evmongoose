#include "emn_ssl.h"
#include "emn_utils.h"

#if (EMN_USE_OPENSSL)

SSL_CTX *emn_ssl_init(const char *cert, const char *key, int type)
{
	SSL_CTX *ctx = NULL;
	const SSL_METHOD *method = NULL;

	SSL_library_init();

	if (type == EMN_TYPE_SERVER)
		method = SSLv23_server_method();
	else
		method = SSLv23_client_method();

	/* creates a new SSL_CTX object */
	ctx = SSL_CTX_new(method);
	if (!ctx) {
		emn_log(LOG_ERR, "Failed to create SSL context");
		return NULL;
	}

	/* loads the first certificate stored in file into ctx */
	if (!SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM)) {
		emn_log(LOG_ERR, "OpenSSL Error: loading certificate file failed");
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
		emn_log(LOG_ERR, "OpenSSL Error: loading key failed");
		goto err;
	}
	
	return ctx;
	
err:
	SSL_CTX_free(ctx);
	return NULL;
}

int emn_ssl_accept(struct emn_client *cli)
{	
	cli->ssl = SSL_new(cli->srv->ssl_ctx);
	if (!cli->ssl)
		return -1;

	SSL_set_fd(cli->ssl, cli->sock);

	if (!SSL_accept(cli->ssl)) {
		emn_log(LOG_ERR, "SSL_accept Error: %s", ERR_reason_error_string(ERR_get_error()));
		return -1;
	}

	return 0;
}

ssize_t emn_ssl_read(SSL *ssl, void *buf, size_t count)
{
	int nread = SSL_read(ssl, buf, count);
	if(nread < 0) {
		int err = SSL_get_error(ssl, nread);

		switch(err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			errno = EAGAIN;
			return -1;
		case SSL_ERROR_SYSCALL:
			return -1;
		default:
			emn_log(LOG_ERR, "SSL_read Error: %s", ERR_reason_error_string(ERR_get_error()));
			errno = EPROTO;
			return -1;
		}
	}
	return (ssize_t)nread;
}

ssize_t emn_ssl_write(SSL *ssl, void *buf, size_t count)
{
	int ret = SSL_write(ssl, buf, count);
	if(ret < 0) {
		int err = SSL_get_error(ssl, ret);

		switch(err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			errno = EAGAIN;
			return -1;
		case SSL_ERROR_SYSCALL:
			return -1;
		case SSL_ERROR_SSL:
			emn_log(LOG_ERR, "SSL_read Error: %s", ERR_reason_error_string(ERR_get_error()));
			errno = EPROTO;
			return -1;
		}
	}
	return (ssize_t)ret;
}

#elif (EMN_USE_CYASSL)
WOLFSSL_CTX *emn_ssl_init(const char *cert, const char *key, int type)
{
	WOLFSSL_CTX *ctx = NULL;
	WOLFSSL_METHOD *method = NULL;

	/* Initialize wolfSSL */
	wolfSSL_Init();

	if (type == EMN_TYPE_SERVER)
		method = wolfSSLv23_server_method();
	else
		method = wolfSSLv23_client_method();

	/* Create the WOLFSSL_CTX */
	ctx = wolfSSL_CTX_new(method);
	if (!ctx) {
		emn_log(LOG_ERR, "Failed to create wolfSSL context");
		return NULL;
	}

	/* Load server certificates into WOLFSSL_CTX */
	if (wolfSSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
		emn_log(LOG_ERR, "wolfSSL Error: loading certificate file failed");
		goto err;
	}

	/* Load keys */
	if (wolfSSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) != SSL_SUCCESS){
		emn_log(LOG_ERR, "wolfSSL Error: loading key failed");
		goto err;
	}

	return ctx;
	
err:
	wolfSSL_CTX_free(ctx);
	wolfSSL_Cleanup();
	return NULL;
}

int emn_ssl_accept(struct emn_client *cli)
{
	cli->ssl = wolfSSL_new(cli->srv->ssl_ctx);
	if (!cli->ssl)
		return -1;

	wolfSSL_set_fd(cli->ssl, cli->sock);

	return 0;
}

ssize_t emn_ssl_read(WOLFSSL *ssl, void *buf, size_t count)
{
	int nread = wolfSSL_read(ssl, buf, count);
	if(nread < 0) {
		int err = wolfSSL_get_error(ssl, nread);

		switch(err) {
		case SSL_ERROR_ZERO_RETURN: /* no more data */
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:	/* there's data pending, re-invoke wolfSSL_read() */
			errno = EAGAIN;
			return -1;
		default:
			emn_log(LOG_ERR, "wolfSSL_read Error: %d:%s", errno, wolfSSL_ERR_reason_error_string(err));
			errno = EPROTO;
			return -1;
		}
	}
	return (ssize_t)nread;
}

ssize_t emn_ssl_write(WOLFSSL *ssl, void *buf, size_t count)
{
	int ret = wolfSSL_write(ssl, buf, count);
	if(ret < 0) {
		int err = wolfSSL_get_error(ssl, ret);

		switch(err) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:	/* there's data pending, re-invoke wolfSSL_write() */
			errno = EAGAIN;
			return -1;
		default:
			emn_log(LOG_ERR, "wolfSSL_write Error: %d:%s", errno, wolfSSL_ERR_reason_error_string(err));
			errno = EPROTO;
			return -1;
		}
	}
	return (ssize_t)ret;
}

#endif
