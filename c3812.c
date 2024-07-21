xmlIOHTTPCloseWrite( void * context, const char * http_mthd ) {

    int				close_rc = -1;
    int				http_rtn = 0;
    int				content_lgth = 0;
    xmlIOHTTPWriteCtxtPtr	ctxt = context;

    char *			http_content = NULL;
    char *			content_encoding = NULL;
    char *			content_type = (char *) "text/xml";
    void *			http_ctxt = NULL;

    if ( ( ctxt == NULL ) || ( http_mthd == NULL ) )
	return ( -1 );

    /*  Retrieve the content from the appropriate buffer  */

#ifdef HAVE_ZLIB_H

    if ( ctxt->compression > 0 ) {
	content_lgth = xmlZMemBuffGetContent( ctxt->doc_buff, &http_content );
	content_encoding = (char *) "Content-Encoding: gzip";
    }
    else
#endif
    {
	/*  Pull the data out of the memory output buffer  */

	xmlOutputBufferPtr	dctxt = ctxt->doc_buff;
	http_content = (char *) xmlBufContent(dctxt->buffer);
	content_lgth = xmlBufUse(dctxt->buffer);
    }

    if ( http_content == NULL ) {
	xmlChar msg[500];
	xmlStrPrintf(msg, 500,
		     (const xmlChar *) "xmlIOHTTPCloseWrite:  %s '%s' %s '%s'.\n",
		     "Error retrieving content.\nUnable to",
		     http_mthd, "data to URI", ctxt->uri );
	xmlIOErr(XML_IO_WRITE, (const char *) msg);
    }

    else {

	http_ctxt = xmlNanoHTTPMethod( ctxt->uri, http_mthd, http_content,
					&content_type, content_encoding,
					content_lgth );

	if ( http_ctxt != NULL ) {
#ifdef DEBUG_HTTP
	    /*  If testing/debugging - dump reply with request content  */

	    FILE *	tst_file = NULL;
	    char	buffer[ 4096 ];
	    char *	dump_name = NULL;
	    int		avail;

	    xmlGenericError( xmlGenericErrorContext,
			"xmlNanoHTTPCloseWrite:  HTTP %s to\n%s returned %d.\n",
			http_mthd, ctxt->uri,
			xmlNanoHTTPReturnCode( http_ctxt ) );

	    /*
	    **  Since either content or reply may be gzipped,
	    **  dump them to separate files instead of the
	    **  standard error context.
	    */

	    dump_name = tempnam( NULL, "lxml" );
	    if ( dump_name != NULL ) {
		(void)snprintf( buffer, sizeof(buffer), "%s.content", dump_name );

		tst_file = fopen( buffer, "wb" );
		if ( tst_file != NULL ) {
		    xmlGenericError( xmlGenericErrorContext,
			"Transmitted content saved in file:  %s\n", buffer );

		    fwrite( http_content, sizeof( char ),
					content_lgth, tst_file );
		    fclose( tst_file );
		}

		(void)snprintf( buffer, sizeof(buffer), "%s.reply", dump_name );
		tst_file = fopen( buffer, "wb" );
		if ( tst_file != NULL ) {
		    xmlGenericError( xmlGenericErrorContext,
			"Reply content saved in file:  %s\n", buffer );


		    while ( (avail = xmlNanoHTTPRead( http_ctxt,
					buffer, sizeof( buffer ) )) > 0 ) {

			fwrite( buffer, sizeof( char ), avail, tst_file );
		    }

		    fclose( tst_file );
		}

		free( dump_name );
	    }
#endif  /*  DEBUG_HTTP  */

	    http_rtn = xmlNanoHTTPReturnCode( http_ctxt );
	    if ( ( http_rtn >= 200 ) && ( http_rtn < 300 ) )
		close_rc = 0;
	    else {
                xmlChar msg[500];
                xmlStrPrintf(msg, 500,
    (const xmlChar *) "xmlIOHTTPCloseWrite: HTTP '%s' of %d %s\n'%s' %s %d\n",
			    http_mthd, content_lgth,
			    "bytes to URI", ctxt->uri,
			    "failed.  HTTP return code:", http_rtn );
		xmlIOErr(XML_IO_WRITE, (const char *) msg);
            }

	    xmlNanoHTTPClose( http_ctxt );
	    xmlFree( content_type );
	}
    }

    /*  Final cleanups  */

    xmlFreeHTTPWriteCtxt( ctxt );

    return ( close_rc );
}