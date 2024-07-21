xmlCharEncInFunc(xmlCharEncodingHandler * handler, xmlBufferPtr out,
                 xmlBufferPtr in)
{
    int ret = -2;
    int written;
    int toconv;

    if (handler == NULL)
        return (-1);
    if (out == NULL)
        return (-1);
    if (in == NULL)
        return (-1);

    toconv = in->use;
    if (toconv == 0)
        return (0);
    written = out->size - out->use;
    if (toconv * 2 >= written) {
        xmlBufferGrow(out, out->size + toconv * 2);
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
        if (ret == -1)
            ret = -3;
    }
#endif /* LIBXML_ICONV_ENABLED */
#ifdef LIBXML_ICU_ENABLED
    else if (handler->uconv_in != NULL) {
        ret = xmlUconvWrapper(handler->uconv_in, 1, &out->content[out->use],
                              &written, in->content, &toconv);
        xmlBufferShrink(in, toconv);
        out->use += written;
        out->content[out->use] = 0;
        if (ret == -1)
            ret = -3;
    }
#endif /* LIBXML_ICU_ENABLED */
    switch (ret) {
        case 0:
#ifdef DEBUG_ENCODING
            xmlGenericError(xmlGenericErrorContext,
                            "converted %d bytes to %d bytes of input\n",
                            toconv, written);
#endif
            break;
        case -1:
#ifdef DEBUG_ENCODING
            xmlGenericError(xmlGenericErrorContext,
                         "converted %d bytes to %d bytes of input, %d left\n",
                            toconv, written, in->use);
#endif
            break;
        case -3:
#ifdef DEBUG_ENCODING
            xmlGenericError(xmlGenericErrorContext,
                        "converted %d bytes to %d bytes of input, %d left\n",
                            toconv, written, in->use);
#endif
            break;
        case -2: {
            char buf[50];

	    snprintf(&buf[0], 49, "0x%02X 0x%02X 0x%02X 0x%02X",
		     in->content[0], in->content[1],
		     in->content[2], in->content[3]);
	    buf[49] = 0;
	    xmlEncodingErr(XML_I18N_CONV_FAILED,
		    "input conversion failed due to input error, bytes %s\n",
		           buf);
        }
    }
    /*
     * Ignore when input buffer is not on a boundary
     */
    if (ret == -3)
        ret = 0;
    return (written? written : ret);
}