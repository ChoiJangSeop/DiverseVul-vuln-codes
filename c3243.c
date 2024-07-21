xmlParseXMLDecl(xmlParserCtxtPtr ctxt) {
    xmlChar *version;

    /*
     * This value for standalone indicates that the document has an
     * XML declaration but it does not have a standalone attribute.
     * It will be overwritten later if a standalone attribute is found.
     */
    ctxt->input->standalone = -2;

    /*
     * We know that '<?xml' is here.
     */
    SKIP(5);

    if (!IS_BLANK_CH(RAW)) {
	xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
	               "Blank needed after '<?xml'\n");
    }
    SKIP_BLANKS;

    /*
     * We must have the VersionInfo here.
     */
    version = xmlParseVersionInfo(ctxt);
    if (version == NULL) {
	xmlFatalErr(ctxt, XML_ERR_VERSION_MISSING, NULL);
    } else {
	if (!xmlStrEqual(version, (const xmlChar *) XML_DEFAULT_VERSION)) {
	    /*
	     * Changed here for XML-1.0 5th edition
	     */
	    if (ctxt->options & XML_PARSE_OLD10) {
		xmlFatalErrMsgStr(ctxt, XML_ERR_UNKNOWN_VERSION,
			          "Unsupported version '%s'\n",
			          version);
	    } else {
	        if ((version[0] == '1') && ((version[1] == '.'))) {
		    xmlWarningMsg(ctxt, XML_WAR_UNKNOWN_VERSION,
		                  "Unsupported version '%s'\n",
				  version, NULL);
		} else {
		    xmlFatalErrMsgStr(ctxt, XML_ERR_UNKNOWN_VERSION,
				      "Unsupported version '%s'\n",
				      version);
		}
	    }
	}
	if (ctxt->version != NULL)
	    xmlFree((void *) ctxt->version);
	ctxt->version = version;
    }

    /*
     * We may have the encoding declaration
     */
    if (!IS_BLANK_CH(RAW)) {
        if ((RAW == '?') && (NXT(1) == '>')) {
	    SKIP(2);
	    return;
	}
	xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED, "Blank needed here\n");
    }
    xmlParseEncodingDecl(ctxt);
    if (ctxt->errNo == XML_ERR_UNSUPPORTED_ENCODING) {
	/*
	 * The XML REC instructs us to stop parsing right here
	 */
        return;
    }

    /*
     * We may have the standalone status.
     */
    if ((ctxt->input->encoding != NULL) && (!IS_BLANK_CH(RAW))) {
        if ((RAW == '?') && (NXT(1) == '>')) {
	    SKIP(2);
	    return;
	}
	xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED, "Blank needed here\n");
    }

    /*
     * We can grow the input buffer freely at that point
     */
    GROW;

    SKIP_BLANKS;
    ctxt->input->standalone = xmlParseSDDecl(ctxt);

    SKIP_BLANKS;
    if ((RAW == '?') && (NXT(1) == '>')) {
        SKIP(2);
    } else if (RAW == '>') {
        /* Deprecated old WD ... */
	xmlFatalErr(ctxt, XML_ERR_XMLDECL_NOT_FINISHED, NULL);
	NEXT;
    } else {
	xmlFatalErr(ctxt, XML_ERR_XMLDECL_NOT_FINISHED, NULL);
	MOVETO_ENDTAG(CUR_PTR);
	NEXT;
    }
}