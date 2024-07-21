RedStreamSslStatus red_stream_ssl_accept(RedStream *stream)
{
    int ssl_error;
    int return_code;

    return_code = SSL_accept(stream->priv->ssl);
    if (return_code == 1) {
        return RED_STREAM_SSL_STATUS_OK;
    }

    ssl_error = SSL_get_error(stream->priv->ssl, return_code);
    if (return_code == -1 && (ssl_error == SSL_ERROR_WANT_READ ||
                              ssl_error == SSL_ERROR_WANT_WRITE)) {
        if (ssl_error == SSL_ERROR_WANT_READ) {
            return RED_STREAM_SSL_STATUS_WAIT_FOR_READ;
        } else {
            return RED_STREAM_SSL_STATUS_WAIT_FOR_WRITE;
        }
    }

    red_dump_openssl_errors();
    spice_warning("SSL_accept failed, error=%d", ssl_error);
    SSL_free(stream->priv->ssl);
    stream->priv->ssl = NULL;

    return RED_STREAM_SSL_STATUS_ERROR;
}