static pj_status_t STATUS_FROM_SSL_ERR2(char *action, pj_ssl_sock_t *ssock,
					int ret, int err, int len)
{
    unsigned long ssl_err = err;

    if (err == SSL_ERROR_SSL) {
	ssl_err = ERR_peek_error();
    }

    /* Dig for more from OpenSSL error queue */
    SSLLogErrors(action, ret, err, len, ssock);

    ssock->last_err = ssl_err;
    return GET_STATUS_FROM_SSL_ERR(ssl_err);
}