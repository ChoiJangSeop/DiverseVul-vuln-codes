MONGO_EXPORT int mongo_run_command( mongo *conn, const char *db, const bson *command,
                                    bson *out ) {
    int ret = MONGO_OK;
    bson response = {NULL, 0};
    bson fields;
    int sl = strlen( db );
    char *ns = bson_malloc( sl + 5 + 1 ); /* ".$cmd" + nul */
    int res, success = 0;

    strcpy( ns, db );
    strcpy( ns+sl, ".$cmd" );

    res = mongo_find_one( conn, ns, command, bson_empty( &fields ), &response );
    bson_free( ns );

    if( res != MONGO_OK )
        ret = MONGO_ERROR;
    else {
        bson_iterator it;
        if( bson_find( &it, &response, "ok" ) )
            success = bson_iterator_bool( &it );

        if( !success ) {
            conn->err = MONGO_COMMAND_FAILED;
            bson_destroy( &response );
            ret = MONGO_ERROR;
        }
        else {
            if( out )
                *out = response;
            else
                bson_destroy( &response );
        }
    }
    return ret;
}