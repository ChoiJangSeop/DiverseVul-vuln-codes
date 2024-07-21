xmlZMemBuffAppend( xmlZMemBuffPtr buff, const char * src, int len ) {

    int		z_err;
    size_t	min_accept;

    if ( ( buff == NULL ) || ( src == NULL ) )
	return ( -1 );

    buff->zctrl.avail_in = len;
    buff->zctrl.next_in  = (unsigned char *)src;
    while ( buff->zctrl.avail_in > 0 ) {
	/*
	**  Extend the buffer prior to deflate call if a reasonable amount
	**  of output buffer space is not available.
	*/
	min_accept = buff->zctrl.avail_in / DFLT_ZLIB_RATIO;
	if ( buff->zctrl.avail_out <= min_accept ) {
	    if ( xmlZMemBuffExtend( buff, buff->size ) == -1 )
		return ( -1 );
	}

	z_err = deflate( &buff->zctrl, Z_NO_FLUSH );
	if ( z_err != Z_OK ) {
	    xmlChar msg[500];
	    xmlStrPrintf(msg, 500,
			(const xmlChar *) "xmlZMemBuffAppend:  %s %d %s - %d",
			"Compression error while appending",
			len, "bytes to buffer.  ZLIB error", z_err );
	    xmlIOErr(XML_IO_WRITE, (const char *) msg);
	    return ( -1 );
	}
    }

    buff->crc = crc32( buff->crc, (unsigned char *)src, len );

    return ( len );
}