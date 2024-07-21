xsltLoadStyleDocument(xsltStylesheetPtr style, const xmlChar *URI) {
    xsltDocumentPtr ret;
    xmlDocPtr doc;
    xsltSecurityPrefsPtr sec;

    if ((style == NULL) || (URI == NULL))
	return(NULL);

    /*
     * Security framework check
     */
    sec = xsltGetDefaultSecurityPrefs();
    if (sec != NULL) {
	int res;

	res = xsltCheckRead(sec, NULL, URI);
	if (res == 0) {
	    xsltTransformError(NULL, NULL, NULL,
		 "xsltLoadStyleDocument: read rights for %s denied\n",
			     URI);
	    return(NULL);
	}
    }

    /*
     * Walk the context list to find the document if preparsed
     */
    ret = style->docList;
    while (ret != NULL) {
	if ((ret->doc != NULL) && (ret->doc->URL != NULL) &&
	    (xmlStrEqual(ret->doc->URL, URI)))
	    return(ret);
	ret = ret->next;
    }

    doc = xsltDocDefaultLoader(URI, style->dict, XSLT_PARSE_OPTIONS,
                               (void *) style, XSLT_LOAD_STYLESHEET);
    if (doc == NULL)
	return(NULL);

    ret = xsltNewStyleDocument(style, doc);
    return(ret);
}