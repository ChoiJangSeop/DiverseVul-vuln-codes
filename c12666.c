MONGO_EXPORT void __mongo_set_error( mongo *conn, mongo_error_t err, const char *str,
                                     int errcode ) {
    int errstr_size, str_size;

    conn->err = err;
    conn->errcode = errcode;

    if( str ) {
        str_size = strlen( str ) + 1;
        errstr_size = str_size > MONGO_ERR_LEN ? MONGO_ERR_LEN : str_size;
        memcpy( conn->errstr, str, errstr_size );
        conn->errstr[errstr_size-1] = '\0';
    }
}