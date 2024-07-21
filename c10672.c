static void ssl_reset_sock_state(pj_ssl_sock_t *ssock)
{
    ossl_sock_t *ossock = (ossl_sock_t *)ssock;
    /**
     * Avoid calling SSL_shutdown() if handshake wasn't completed.
     * OpenSSL 1.0.2f complains if SSL_shutdown() is called during an
     * SSL handshake, while previous versions always return 0.
     */
    if (ossock->ossl_ssl && SSL_in_init(ossock->ossl_ssl) == 0) {
	int ret = SSL_shutdown(ossock->ossl_ssl);
	if (ret == 0) {
	    /* Flush data to send close notify. */
	    flush_circ_buf_output(ssock, &ssock->shutdown_op_key, 0, 0);
	}
    }

    pj_lock_acquire(ssock->write_mutex);
    ssock->ssl_state = SSL_STATE_NULL;
    pj_lock_release(ssock->write_mutex);

    ssl_close_sockets(ssock);

    /* Upon error, OpenSSL may leave any error description in the thread 
     * error queue, which sometime may cause next call to SSL API returning
     * false error alarm, e.g: in Linux, SSL_CTX_use_certificate_chain_file()
     * returning false error after a handshake error (in different SSL_CTX!).
     * For now, just clear thread error queue here.
     */
    ERR_clear_error();
}