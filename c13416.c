static int ssl_handle_possible_reconnect( mbedtls_ssl_context *ssl )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    if( ssl->conf->f_cookie_write == NULL ||
        ssl->conf->f_cookie_check == NULL )
    {
        /* If we can't use cookies to verify reachability of the peer,
         * drop the record. */
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "no cookie callbacks, "
                                    "can't check reconnect validity" ) );
        return( 0 );
    }

    ret = ssl_check_dtls_clihlo_cookie(
            ssl->conf->f_cookie_write,
            ssl->conf->f_cookie_check,
            ssl->conf->p_cookie,
            ssl->cli_id, ssl->cli_id_len,
            ssl->in_buf, ssl->in_left,
            ssl->out_buf, MBEDTLS_SSL_OUT_CONTENT_LEN, &len );

    MBEDTLS_SSL_DEBUG_RET( 2, "ssl_check_dtls_clihlo_cookie", ret );

    if( ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED )
    {
        int send_ret;
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "sending HelloVerifyRequest" ) );
        MBEDTLS_SSL_DEBUG_BUF( 4, "output record sent to network",
                                  ssl->out_buf, len );
        /* Don't check write errors as we can't do anything here.
         * If the error is permanent we'll catch it later,
         * if it's not, then hopefully it'll work next time. */
        send_ret = ssl->f_send( ssl->p_bio, ssl->out_buf, len );
        MBEDTLS_SSL_DEBUG_RET( 2, "ssl->f_send", send_ret );
        (void) send_ret;

        return( 0 );
    }

    if( ret == 0 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "cookie is valid, resetting context" ) );
        if( ( ret = mbedtls_ssl_session_reset_int( ssl, 1 ) ) != 0 )
        {
            MBEDTLS_SSL_DEBUG_RET( 1, "reset", ret );
            return( ret );
        }

        return( MBEDTLS_ERR_SSL_CLIENT_RECONNECT );
    }

    return( ret );
}