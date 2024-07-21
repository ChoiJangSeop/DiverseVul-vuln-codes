xmlXPathErrMemory(xmlXPathContextPtr ctxt, const char *extra)
{
    if (ctxt != NULL) {
        if (extra) {
            xmlChar buf[200];

            xmlStrPrintf(buf, 200,
                         BAD_CAST "Memory allocation failed : %s\n",
                         extra);
            ctxt->lastError.message = (char *) xmlStrdup(buf);
        } else {
            ctxt->lastError.message = (char *)
	       xmlStrdup(BAD_CAST "Memory allocation failed\n");
        }
        ctxt->lastError.domain = XML_FROM_XPATH;
        ctxt->lastError.code = XML_ERR_NO_MEMORY;
	if (ctxt->error != NULL)
	    ctxt->error(ctxt->userData, &ctxt->lastError);
    } else {
        if (extra)
            __xmlRaiseError(NULL, NULL, NULL,
                            NULL, NULL, XML_FROM_XPATH,
                            XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0,
                            extra, NULL, NULL, 0, 0,
                            "Memory allocation failed : %s\n", extra);
        else
            __xmlRaiseError(NULL, NULL, NULL,
                            NULL, NULL, XML_FROM_XPATH,
                            XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0,
                            NULL, NULL, NULL, 0, 0,
                            "Memory allocation failed\n");
    }
}