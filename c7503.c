xsltLoadDocument(xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltDocumentPtr ret;
    xmlDocPtr doc;

    if ((ctxt == NULL) || (URI == NULL))
	return(NULL);

    /*
     * Security framework check
     */
    if (ctxt->sec != NULL) {
	int res;

	res = xsltCheckRead(ctxt->sec, ctxt, URI);
	if (res == 0) {
	    xsltTransformError(ctxt, NULL, NULL,
		 "xsltLoadDocument: read rights for %s denied\n",
			     URI);
	    return(NULL);
	}
    }

    /*
     * Walk the context list to find the document if preparsed
     */
    ret = ctxt->docList;
    while (ret != NULL) {
	if ((ret->doc != NULL) && (ret->doc->URL != NULL) &&
	    (xmlStrEqual(ret->doc->URL, URI)))
	    return(ret);
	ret = ret->next;
    }

    doc = xsltDocDefaultLoader(URI, ctxt->dict, ctxt->parserOptions,
                               (void *) ctxt, XSLT_LOAD_DOCUMENT);

    if (doc == NULL)
	return(NULL);

    if (ctxt->xinclude != 0) {
#ifdef LIBXML_XINCLUDE_ENABLED
#if LIBXML_VERSION >= 20603
	xmlXIncludeProcessFlags(doc, ctxt->parserOptions);
#else
	xmlXIncludeProcess(doc);
#endif
#else
	xsltTransformError(ctxt, NULL, NULL,
	    "xsltLoadDocument(%s) : XInclude processing not compiled in\n",
	                 URI);
#endif
    }
    /*
     * Apply white-space stripping if asked for
     */
    if (xsltNeedElemSpaceHandling(ctxt))
	xsltApplyStripSpaces(ctxt, xmlDocGetRootElement(doc));
    if (ctxt->debugStatus == XSLT_DEBUG_NONE)
	xmlXPathOrderDocElems(doc);

    ret = xsltNewDocument(ctxt, doc);
    return(ret);
}