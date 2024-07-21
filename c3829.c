pushParseTest(const char *filename, const char *result,
             const char *err ATTRIBUTE_UNUSED,
	     int options) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;
    const char *base;
    int size, res;
    int cur = 0;

    nb_tests++;
    /*
     * load the document in memory and work from there.
     */
    if (loadMem(filename, &base, &size) != 0) {
        fprintf(stderr, "Failed to load %s\n", filename);
	return(-1);
    }

#ifdef LIBXML_HTML_ENABLED
    if (options & XML_PARSE_HTML)
	ctxt = htmlCreatePushParserCtxt(NULL, NULL, base + cur, 4, filename,
	                                XML_CHAR_ENCODING_NONE);
    else
#endif
    ctxt = xmlCreatePushParserCtxt(NULL, NULL, base + cur, 4, filename);
    xmlCtxtUseOptions(ctxt, options);
    cur += 4;
    while (cur < size) {
        if (cur + 1024 >= size) {
#ifdef LIBXML_HTML_ENABLED
	    if (options & XML_PARSE_HTML)
		htmlParseChunk(ctxt, base + cur, size - cur, 1);
	    else
#endif
	    xmlParseChunk(ctxt, base + cur, size - cur, 1);
	    break;
	} else {
#ifdef LIBXML_HTML_ENABLED
	    if (options & XML_PARSE_HTML)
		htmlParseChunk(ctxt, base + cur, 1024, 0);
	    else
#endif
	    xmlParseChunk(ctxt, base + cur, 1024, 0);
	    cur += 1024;
	}
    }
    doc = ctxt->myDoc;
#ifdef LIBXML_HTML_ENABLED
    if (options & XML_PARSE_HTML)
        res = 1;
    else
#endif
    res = ctxt->wellFormed;
    xmlFreeParserCtxt(ctxt);
    free((char *)base);
    if (!res) {
	xmlFreeDoc(doc);
	fprintf(stderr, "Failed to parse %s\n", filename);
	return(-1);
    }
#ifdef LIBXML_HTML_ENABLED
    if (options & XML_PARSE_HTML)
	htmlDocDumpMemory(doc, (xmlChar **) &base, &size);
    else
#endif
    xmlDocDumpMemory(doc, (xmlChar **) &base, &size);
    xmlFreeDoc(doc);
    res = compareFileMem(result, base, size);
    if ((base == NULL) || (res != 0)) {
	if (base != NULL)
	    xmlFree((char *)base);
        fprintf(stderr, "Result for %s failed in %s\n", filename, result);
	return(-1);
    }
    xmlFree((char *)base);
    if (err != NULL) {
	res = compareFileMem(err, testErrors, testErrorsSize);
	if (res != 0) {
	    fprintf(stderr, "Error for %s failed\n", filename);
	    return(-1);
	}
    }
    return(0);
}