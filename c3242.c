xmlSwitchEncoding(xmlParserCtxtPtr ctxt, xmlCharEncoding enc)
{
    xmlCharEncodingHandlerPtr handler;
    int len = -1;

    if (ctxt == NULL) return(-1);
    switch (enc) {
	case XML_CHAR_ENCODING_ERROR:
	    __xmlErrEncoding(ctxt, XML_ERR_UNKNOWN_ENCODING,
	                   "encoding unknown\n", NULL, NULL);
	    return(-1);
	case XML_CHAR_ENCODING_NONE:
	    /* let's assume it's UTF-8 without the XML decl */
	    ctxt->charset = XML_CHAR_ENCODING_UTF8;
	    return(0);
	case XML_CHAR_ENCODING_UTF8:
	    /* default encoding, no conversion should be needed */
	    ctxt->charset = XML_CHAR_ENCODING_UTF8;

	    /*
	     * Errata on XML-1.0 June 20 2001
	     * Specific handling of the Byte Order Mark for
	     * UTF-8
	     */
	    if ((ctxt->input != NULL) &&
		(ctxt->input->cur[0] == 0xEF) &&
		(ctxt->input->cur[1] == 0xBB) &&
		(ctxt->input->cur[2] == 0xBF)) {
		ctxt->input->cur += 3;
	    }
	    return(0);
    case XML_CHAR_ENCODING_UTF16LE:
    case XML_CHAR_ENCODING_UTF16BE:
        /*The raw input characters are encoded
         *in UTF-16. As we expect this function
         *to be called after xmlCharEncInFunc, we expect
         *ctxt->input->cur to contain UTF-8 encoded characters.
         *So the raw UTF16 Byte Order Mark
         *has also been converted into
         *an UTF-8 BOM. Let's skip that BOM.
         */
        if ((ctxt->input != NULL) && (ctxt->input->cur != NULL) &&
            (ctxt->input->cur[0] == 0xEF) &&
            (ctxt->input->cur[1] == 0xBB) &&
            (ctxt->input->cur[2] == 0xBF)) {
            ctxt->input->cur += 3;
        }
        len = 90;
	break;
    case XML_CHAR_ENCODING_UCS2:
        len = 90;
	break;
    case XML_CHAR_ENCODING_UCS4BE:
    case XML_CHAR_ENCODING_UCS4LE:
    case XML_CHAR_ENCODING_UCS4_2143:
    case XML_CHAR_ENCODING_UCS4_3412:
        len = 180;
	break;
    case XML_CHAR_ENCODING_EBCDIC:
    case XML_CHAR_ENCODING_8859_1:
    case XML_CHAR_ENCODING_8859_2:
    case XML_CHAR_ENCODING_8859_3:
    case XML_CHAR_ENCODING_8859_4:
    case XML_CHAR_ENCODING_8859_5:
    case XML_CHAR_ENCODING_8859_6:
    case XML_CHAR_ENCODING_8859_7:
    case XML_CHAR_ENCODING_8859_8:
    case XML_CHAR_ENCODING_8859_9:
    case XML_CHAR_ENCODING_ASCII:
    case XML_CHAR_ENCODING_2022_JP:
    case XML_CHAR_ENCODING_SHIFT_JIS:
    case XML_CHAR_ENCODING_EUC_JP:
        len = 45;
	break;
    }
    handler = xmlGetCharEncodingHandler(enc);
    if (handler == NULL) {
	/*
	 * Default handlers.
	 */
	switch (enc) {
	    case XML_CHAR_ENCODING_ASCII:
		/* default encoding, no conversion should be needed */
		ctxt->charset = XML_CHAR_ENCODING_UTF8;
		return(0);
	    case XML_CHAR_ENCODING_UTF16LE:
		break;
	    case XML_CHAR_ENCODING_UTF16BE:
		break;
	    case XML_CHAR_ENCODING_UCS4LE:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "USC4 little endian", NULL);
		break;
	    case XML_CHAR_ENCODING_UCS4BE:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "USC4 big endian", NULL);
		break;
	    case XML_CHAR_ENCODING_EBCDIC:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "EBCDIC", NULL);
		break;
	    case XML_CHAR_ENCODING_UCS4_2143:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "UCS4 2143", NULL);
		break;
	    case XML_CHAR_ENCODING_UCS4_3412:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "UCS4 3412", NULL);
		break;
	    case XML_CHAR_ENCODING_UCS2:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "UCS2", NULL);
		break;
	    case XML_CHAR_ENCODING_8859_1:
	    case XML_CHAR_ENCODING_8859_2:
	    case XML_CHAR_ENCODING_8859_3:
	    case XML_CHAR_ENCODING_8859_4:
	    case XML_CHAR_ENCODING_8859_5:
	    case XML_CHAR_ENCODING_8859_6:
	    case XML_CHAR_ENCODING_8859_7:
	    case XML_CHAR_ENCODING_8859_8:
	    case XML_CHAR_ENCODING_8859_9:
		/*
		 * We used to keep the internal content in the
		 * document encoding however this turns being unmaintainable
		 * So xmlGetCharEncodingHandler() will return non-null
		 * values for this now.
		 */
		if ((ctxt->inputNr == 1) &&
		    (ctxt->encoding == NULL) &&
		    (ctxt->input != NULL) &&
		    (ctxt->input->encoding != NULL)) {
		    ctxt->encoding = xmlStrdup(ctxt->input->encoding);
		}
		ctxt->charset = enc;
		return(0);
	    case XML_CHAR_ENCODING_2022_JP:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "ISO-2022-JP", NULL);
		break;
	    case XML_CHAR_ENCODING_SHIFT_JIS:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "Shift_JIS", NULL);
		break;
	    case XML_CHAR_ENCODING_EUC_JP:
		__xmlErrEncoding(ctxt, XML_ERR_UNSUPPORTED_ENCODING,
			       "encoding not supported %s\n",
			       BAD_CAST "EUC-JP", NULL);
		break;
	    default:
	        break;
	}
    }
    if (handler == NULL)
	return(-1);
    ctxt->charset = XML_CHAR_ENCODING_UTF8;
    return(xmlSwitchToEncodingInt(ctxt, handler, len));
}