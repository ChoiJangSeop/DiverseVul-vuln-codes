xmlCharEncFirstLineInt(xmlCharEncodingHandler *handler, xmlBufferPtr out,
                       xmlBufferPtr in, int len) {
    int ret = -2;
    int written;
    int toconv;

    if (handler == NULL) return(-1);
    if (out == NULL) return(-1);
    if (in == NULL) return(-1);

    /* calculate space available */
    written = out->size - out->use;
    toconv = in->use;
    /*
     * echo '<?xml version="1.0" encoding="UCS4"?>' | wc -c => 38
     * 45 chars should be sufficient to reach the end of the encoding
     * declaration without going too far inside the document content.
     * on UTF-16 this means 90bytes, on UCS4 this means 180
     * The actual value depending on guessed encoding is passed as @len
     * if provided
     */
    if (len >= 0) {
        if (toconv > len)
            toconv = len;
    } else {
        if (toconv > 180)
            toconv = 180;
    }
    if (toconv * 2 >= written) {
        xmlBufferGrow(out, toconv);
	written = out->size - out->use - 1;
    }

    if (handler->input != NULL) {
	ret = handler->input(&out->content[out->use], &written,
	                     in->content, &toconv);
	xmlBufferShrink(in, toconv);
	out->use += written;
	out->content[out->use] = 0;
    }
#ifdef LIBXML_ICONV_ENABLED
    else if (handler->iconv_in != NULL) {
	ret = xmlIconvWrapper(handler->iconv_in, &out->content[out->use],
	                      &written, in->content, &toconv);
	xmlBufferShrink(in, toconv);
	out->use += written;
	out->content[out->use] = 0;
	if (ret == -1) ret = -3;
    }
#endif /* LIBXML_ICONV_ENABLED */
#ifdef LIBXML_ICU_ENABLED
    else if (handler->uconv_in != NULL) {
	ret = xmlUconvWrapper(handler->uconv_in, 1, &out->content[out->use],
	                      &written, in->content, &toconv);
	xmlBufferShrink(in, toconv);
	out->use += written;
	out->content[out->use] = 0;
	if (ret == -1) ret = -3;
    }
#endif /* LIBXML_ICU_ENABLED */
#ifdef DEBUG_ENCODING
    switch (ret) {
        case 0:
	    xmlGenericError(xmlGenericErrorContext,
		    "converted %d bytes to %d bytes of input\n",
	            toconv, written);
	    break;
        case -1:
	    xmlGenericError(xmlGenericErrorContext,"converted %d bytes to %d bytes of input, %d left\n",
	            toconv, written, in->use);
	    break;
        case -2:
	    xmlGenericError(xmlGenericErrorContext,
		    "input conversion failed due to input error\n");
	    break;
        case -3:
	    xmlGenericError(xmlGenericErrorContext,"converted %d bytes to %d bytes of input, %d left\n",
	            toconv, written, in->use);
	    break;
	default:
	    xmlGenericError(xmlGenericErrorContext,"Unknown input conversion failed %d\n", ret);
    }
#endif /* DEBUG_ENCODING */
    /*
     * Ignore when input buffer is not on a boundary
     */
    if (ret == -3) ret = 0;
    if (ret == -1) ret = 0;
    return(ret);
}