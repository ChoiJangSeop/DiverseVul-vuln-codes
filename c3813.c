xmlCreateZMemBuff( int compression ) {

    int			z_err;
    int			hdr_lgth;
    xmlZMemBuffPtr	buff = NULL;

    if ( ( compression < 1 ) || ( compression > 9 ) )
	return ( NULL );

    /*  Create the control and data areas  */

    buff = xmlMalloc( sizeof( xmlZMemBuff ) );
    if ( buff == NULL ) {
	xmlIOErrMemory("creating buffer context");
	return ( NULL );
    }

    (void)memset( buff, 0, sizeof( xmlZMemBuff ) );
    buff->size = INIT_HTTP_BUFF_SIZE;
    buff->zbuff = xmlMalloc( buff->size );
    if ( buff->zbuff == NULL ) {
	xmlFreeZMemBuff( buff );
	xmlIOErrMemory("creating buffer");
	return ( NULL );
    }

    z_err = deflateInit2( &buff->zctrl, compression, Z_DEFLATED,
			    DFLT_WBITS, DFLT_MEM_LVL, Z_DEFAULT_STRATEGY );
    if ( z_err != Z_OK ) {
	xmlChar msg[500];
	xmlFreeZMemBuff( buff );
	buff = NULL;
	xmlStrPrintf(msg, 500,
		    (const xmlChar *) "xmlCreateZMemBuff:  %s %d\n",
		    "Error initializing compression context.  ZLIB error:",
		    z_err );
	xmlIOErr(XML_IO_WRITE, (const char *) msg);
	return ( NULL );
    }

    /*  Set the header data.  The CRC will be needed for the trailer  */
    buff->crc = crc32( 0L, NULL, 0 );
    hdr_lgth = snprintf( (char *)buff->zbuff, buff->size,
			"%c%c%c%c%c%c%c%c%c%c",
			GZ_MAGIC1, GZ_MAGIC2, Z_DEFLATED,
			0, 0, 0, 0, 0, 0, LXML_ZLIB_OS_CODE );
    buff->zctrl.next_out  = buff->zbuff + hdr_lgth;
    buff->zctrl.avail_out = buff->size - hdr_lgth;

    return ( buff );
}