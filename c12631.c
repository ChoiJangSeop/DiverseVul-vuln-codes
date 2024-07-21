int mongo_env_write_socket( mongo *conn, const void *buf, int len ) {
    const char *cbuf = buf;
#ifdef _WIN32
    int flags = 0;
#else
#ifdef __APPLE__
    int flags = 0;
#else
    int flags = MSG_NOSIGNAL;
#endif
#endif

    while ( len ) {
        int sent = send( conn->sock, cbuf, len, flags );
        if ( sent == -1 ) {
            if (errno == EPIPE)
                conn->connected = 0;
            conn->err = MONGO_IO_ERROR;
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}