int mongo_env_write_socket( mongo *conn, const void *buf, int len ) {
    const char *cbuf = buf;
#ifdef __APPLE__
    int flags = 0;
#else
    int flags = MSG_NOSIGNAL;
#endif

    while ( len ) {
        int sent = send( conn->sock, cbuf, len, flags );
        if ( sent == -1 ) {
            if (errno == EPIPE)
                conn->connected = 0;
            __mongo_set_error( conn, MONGO_IO_ERROR, strerror( errno ), errno );
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}