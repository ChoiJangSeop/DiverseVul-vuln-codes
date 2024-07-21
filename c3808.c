xmlZMemBuffExtend( xmlZMemBuffPtr buff, size_t ext_amt ) {

    int			rc = -1;
    size_t		new_size;
    size_t		cur_used;

    unsigned char *	tmp_ptr = NULL;

    if ( buff == NULL )
	return ( -1 );

    else if ( ext_amt == 0 )
	return ( 0 );

    cur_used = buff->zctrl.next_out - buff->zbuff;
    new_size = buff->size + ext_amt;

#ifdef DEBUG_HTTP
    if ( cur_used > new_size )
	xmlGenericError( xmlGenericErrorContext,
			"xmlZMemBuffExtend:  %s\n%s %d bytes.\n",
			"Buffer overwrite detected during compressed memory",
			"buffer extension.  Overflowed by",
			(cur_used - new_size ) );
#endif

    tmp_ptr = xmlRealloc( buff->zbuff, new_size );
    if ( tmp_ptr != NULL ) {
	rc = 0;
	buff->size  = new_size;
	buff->zbuff = tmp_ptr;
	buff->zctrl.next_out  = tmp_ptr + cur_used;
	buff->zctrl.avail_out = new_size - cur_used;
    }
    else {
	xmlChar msg[500];
	xmlStrPrintf(msg, 500,
		    (const xmlChar *) "xmlZMemBuffExtend:  %s %lu bytes.\n",
		    "Allocation failure extending output buffer to",
		    new_size );
	xmlIOErr(XML_IO_WRITE, (const char *) msg);
    }

    return ( rc );
}