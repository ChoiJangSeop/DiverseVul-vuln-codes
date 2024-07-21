MONGO_EXPORT mongo_cursor *gridfile_get_chunks( gridfile *gfile, int start, int size ) {
    bson_iterator it;
    bson_oid_t id;
    bson gte;
    bson query;
    bson orderby;
    bson command;
    mongo_cursor *cursor;

    bson_find( &it, gfile->meta, "_id" );
    id = *bson_iterator_oid( &it );

    bson_init( &query );
    bson_append_oid( &query, "files_id", &id );
    if ( size == 1 ) {
        bson_append_int( &query, "n", start );
    }
    else {
        bson_init( &gte );
        bson_append_int( &gte, "$gte", start );
        bson_finish( &gte );
        bson_append_bson( &query, "n", &gte );
        bson_destroy( &gte );
    }
    bson_finish( &query );

    bson_init( &orderby );
    bson_append_int( &orderby, "n", 1 );
    bson_finish( &orderby );

    bson_init( &command );
    bson_append_bson( &command, "query", &query );
    bson_append_bson( &command, "orderby", &orderby );
    bson_finish( &command );

    cursor = mongo_find( gfile->gfs->client, gfile->gfs->chunks_ns,
                         &command, NULL, size, 0, 0 );

    bson_destroy( &command );
    bson_destroy( &query );
    bson_destroy( &orderby );

    return cursor;
}