MONGO_EXPORT int mongo_insert_batch( mongo *conn, const char *ns,
                                     const bson **bsons, int count, mongo_write_concern *custom_write_concern,
                                     int flags ) {

    mongo_message *mm;
    mongo_write_concern *write_concern = NULL;
    int i;
    char *data;
    int overhead =  16 + 4 + strlen( ns ) + 1;
    int size = overhead;

    if( mongo_validate_ns( conn, ns ) != MONGO_OK )
        return MONGO_ERROR;

    for( i=0; i<count; i++ ) {
        size += bson_size( bsons[i] );
        if( mongo_bson_valid( conn, bsons[i], 1 ) != MONGO_OK )
            return MONGO_ERROR;
    }

    if( ( size - overhead ) > conn->max_bson_size ) {
        conn->err = MONGO_BSON_TOO_LARGE;
        return MONGO_ERROR;
    }

    if( mongo_choose_write_concern( conn, custom_write_concern,
                                    &write_concern ) == MONGO_ERROR ) {
        return MONGO_ERROR;
    }

    mm = mongo_message_create( size , 0 , 0 , MONGO_OP_INSERT );

    data = &mm->data;
    if( flags & MONGO_CONTINUE_ON_ERROR )
        data = mongo_data_append32( data, &ONE );
    else
        data = mongo_data_append32( data, &ZERO );
    data = mongo_data_append( data, ns, strlen( ns ) + 1 );

    for( i=0; i<count; i++ ) {
        data = mongo_data_append( data, bsons[i]->data, bson_size( bsons[i] ) );
    }

    /* TODO: refactor so that we can send the insert message
     * and the getlasterror messages together. */
    if( write_concern ) {
        if( mongo_message_send( conn, mm ) == MONGO_ERROR ) {
            return MONGO_ERROR;
        }

        return mongo_check_last_error( conn, ns, write_concern );
    }
    else {
        return mongo_message_send( conn, mm );
    }
}