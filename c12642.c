MONGO_EXPORT bson_bool_t mongo_cmd_authenticate( mongo *conn, const char *db, const char *user, const char *pass ) {
    bson from_db;
    bson cmd;
    const char *nonce;
    int result;

    mongo_md5_state_t st;
    mongo_md5_byte_t digest[16];
    char hex_digest[33];

    if( mongo_simple_int_command( conn, db, "getnonce", 1, &from_db ) == MONGO_OK ) {
        bson_iterator it;
        bson_find( &it, &from_db, "nonce" );
        nonce = bson_iterator_string( &it );
    }
    else {
        return MONGO_ERROR;
    }

    mongo_pass_digest( user, pass, hex_digest );

    mongo_md5_init( &st );
    mongo_md5_append( &st, ( const mongo_md5_byte_t * )nonce, strlen( nonce ) );
    mongo_md5_append( &st, ( const mongo_md5_byte_t * )user, strlen( user ) );
    mongo_md5_append( &st, ( const mongo_md5_byte_t * )hex_digest, 32 );
    mongo_md5_finish( &st, digest );
    digest2hex( digest, hex_digest );

    bson_init( &cmd );
    bson_append_int( &cmd, "authenticate", 1 );
    bson_append_string( &cmd, "user", user );
    bson_append_string( &cmd, "nonce", nonce );
    bson_append_string( &cmd, "key", hex_digest );
    bson_finish( &cmd );

    bson_destroy( &from_db );

    result = mongo_run_command( conn, db, &cmd, NULL );

    bson_destroy( &cmd );

    return result;
}