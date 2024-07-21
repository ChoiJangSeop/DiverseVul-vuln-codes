xmlZMemBuffGetContent( xmlZMemBuffPtr buff, char ** data_ref ) {

    int		zlgth = -1;
    int		z_err;

    if ( ( buff == NULL ) || ( data_ref == NULL ) )
	return ( -1 );

    /*  Need to loop until compression output buffers are flushed  */

    do
    {
	z_err = deflate( &buff->zctrl, Z_FINISH );
	if ( z_err == Z_OK ) {
	    /*  In this case Z_OK means more buffer space needed  */

	    if ( xmlZMemBuffExtend( buff, buff->size ) == -1 )
		return ( -1 );
	}
    }
    while ( z_err == Z_OK );

    /*  If the compression state is not Z_STREAM_END, some error occurred  */

    if ( z_err == Z_STREAM_END ) {

	/*  Need to append the gzip data trailer  */

	if ( buff->zctrl.avail_out < ( 2 * sizeof( unsigned long ) ) ) {
	    if ( xmlZMemBuffExtend(buff, (2 * sizeof(unsigned long))) == -1 )
		return ( -1 );
	}

	/*
	**  For whatever reason, the CRC and length data are pushed out
	**  in reverse byte order.  So a memcpy can't be used here.
	*/

	append_reverse_ulong( buff, buff->crc );
	append_reverse_ulong( buff, buff->zctrl.total_in );

	zlgth = buff->zctrl.next_out - buff->zbuff;
	*data_ref = (char *)buff->zbuff;
    }

    else {
	xmlChar msg[500];
	xmlStrPrintf(msg, 500,
		    (const xmlChar *) "xmlZMemBuffGetContent:  %s - %d\n",
		    "Error flushing zlib buffers.  Error code", z_err );
	xmlIOErr(XML_IO_WRITE, (const char *) msg);
    }

    return ( zlgth );
}