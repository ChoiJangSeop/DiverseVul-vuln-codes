xmlIOHTTPWrite( void * context, const char * buffer, int len ) {

    xmlIOHTTPWriteCtxtPtr	ctxt = context;

    if ( ( ctxt == NULL ) || ( ctxt->doc_buff == NULL ) || ( buffer == NULL ) )
	return ( -1 );

    if ( len > 0 ) {

	/*  Use gzwrite or fwrite as previously setup in the open call  */

#ifdef HAVE_ZLIB_H
	if ( ctxt->compression > 0 )
	    len = xmlZMemBuffAppend( ctxt->doc_buff, buffer, len );

	else
#endif
	    len = xmlOutputBufferWrite( ctxt->doc_buff, len, buffer );

	if ( len < 0 ) {
	    xmlChar msg[500];
	    xmlStrPrintf(msg, 500,
			(const xmlChar *) "xmlIOHTTPWrite:  %s\n%s '%s'.\n",
			"Error appending to internal buffer.",
			"Error sending document to URI",
			ctxt->uri );
	    xmlIOErr(XML_IO_WRITE, (const char *) msg);
	}
    }

    return ( len );
}