MONGO_EXPORT void gridfile_write_buffer( gridfile *gfile, const char *data,
        gridfs_offset length ) {

    int bytes_left = 0;
    int data_partial_len = 0;
    int chunks_to_write = 0;
    char *buffer;
    bson *oChunk;
    gridfs_offset to_write = length + gfile->pending_len;

    if ( to_write < DEFAULT_CHUNK_SIZE ) { /* Less than one chunk to write */
        if( gfile->pending_data ) {
            gfile->pending_data = ( char * )bson_realloc( ( void * )gfile->pending_data, gfile->pending_len + to_write );
            memcpy( gfile->pending_data + gfile->pending_len, data, length );
        }
        else if ( to_write > 0 ) {
            gfile->pending_data = ( char * )bson_malloc( to_write );
            memcpy( gfile->pending_data, data, length );
        }
        gfile->pending_len += length;

    }
    else {   /* At least one chunk of data to write */
        chunks_to_write = to_write / DEFAULT_CHUNK_SIZE;
        bytes_left = to_write % DEFAULT_CHUNK_SIZE;

        /* If there's a pending chunk to be written, we need to combine
         * the buffer provided up to DEFAULT_CHUNK_SIZE.
         */
        if ( gfile->pending_len > 0 ) {
            data_partial_len = DEFAULT_CHUNK_SIZE - gfile->pending_len;
            buffer = ( char * )bson_malloc( DEFAULT_CHUNK_SIZE );
            memcpy( buffer, gfile->pending_data, gfile->pending_len );
            memcpy( buffer + gfile->pending_len, data, data_partial_len );

            oChunk = chunk_new( gfile->id, gfile->chunk_num, buffer, DEFAULT_CHUNK_SIZE );
            mongo_insert( gfile->gfs->client, gfile->gfs->chunks_ns, oChunk, NULL );
            chunk_free( oChunk );
            gfile->chunk_num++;
            gfile->length += DEFAULT_CHUNK_SIZE;
            data += data_partial_len;

            chunks_to_write--;

            bson_free( buffer );
        }

        while( chunks_to_write > 0 ) {
            oChunk = chunk_new( gfile->id, gfile->chunk_num, data, DEFAULT_CHUNK_SIZE );
            mongo_insert( gfile->gfs->client, gfile->gfs->chunks_ns, oChunk, NULL );
            chunk_free( oChunk );
            gfile->chunk_num++;
            chunks_to_write--;
            gfile->length += DEFAULT_CHUNK_SIZE;
            data += DEFAULT_CHUNK_SIZE;
        }

        bson_free( gfile->pending_data );

        /* If there are any leftover bytes, store them as pending data. */
        if( bytes_left == 0 )
            gfile->pending_data = NULL;
        else {
            gfile->pending_data = ( char * )bson_malloc( bytes_left );
            memcpy( gfile->pending_data, data, bytes_left );
        }

        gfile->pending_len = bytes_left;
    }
}