int mongo_env_write_socket( mongo *conn, const void *buf, int len ) {
    const char *cbuf = buf;
    int flags = 0;

    while ( len ) {
        int sent = send( conn->sock, cbuf, len, flags );
        if ( sent == -1 ) {
            __mongo_set_error( conn, MONGO_IO_ERROR, NULL, WSAGetLastError() );
            conn->connected = 0;
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}