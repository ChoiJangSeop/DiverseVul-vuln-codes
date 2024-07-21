int mongo_env_read_socket( mongo *conn, void *buf, int len ) {
    char *cbuf = buf;
    while ( len ) {
        int sent = recv( conn->sock, cbuf, len, 0 );
        if ( sent == 0 || sent == -1 ) {
            __mongo_set_error( conn, MONGO_IO_ERROR, strerror( errno ), errno );
            return MONGO_ERROR;
        }
        cbuf += sent;
        len -= sent;
    }

    return MONGO_OK;
}