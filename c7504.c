xsltParseStylesheetImport(xsltStylesheetPtr style, xmlNodePtr cur) {
    int ret = -1;
    xmlDocPtr import = NULL;
    xmlChar *base = NULL;
    xmlChar *uriRef = NULL;
    xmlChar *URI = NULL;
    xsltStylesheetPtr res;
    xsltSecurityPrefsPtr sec;

    if ((cur == NULL) || (style == NULL))
	return (ret);

    uriRef = xmlGetNsProp(cur, (const xmlChar *)"href", NULL);
    if (uriRef == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : missing href attribute\n");
	goto error;
    }

    base = xmlNodeGetBase(style->doc, cur);
    URI = xmlBuildURI(uriRef, base);
    if (URI == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : invalid URI reference %s\n", uriRef);
	goto error;
    }

    res = style;
    while (res != NULL) {
        if (res->doc == NULL)
	    break;
	if (xmlStrEqual(res->doc->URL, URI)) {
	    xsltTransformError(NULL, style, cur,
	       "xsl:import : recursion detected on imported URL %s\n", URI);
	    goto error;
	}
	res = res->parent;
    }

    /*
     * Security framework check
     */
    sec = xsltGetDefaultSecurityPrefs();
    if (sec != NULL) {
	int secres;

	secres = xsltCheckRead(sec, NULL, URI);
	if (secres == 0) {
	    xsltTransformError(NULL, NULL, NULL,
		 "xsl:import: read rights for %s denied\n",
			     URI);
	    goto error;
	}
    }

    import = xsltDocDefaultLoader(URI, style->dict, XSLT_PARSE_OPTIONS,
                                  (void *) style, XSLT_LOAD_STYLESHEET);
    if (import == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:import : unable to load %s\n", URI);
	goto error;
    }

    res = xsltParseStylesheetImportedDoc(import, style);
    if (res != NULL) {
	res->next = style->imports;
	style->imports = res;
	if (style->parent == NULL) {
	    xsltFixImportedCompSteps(style, res);
	}
	ret = 0;
    } else {
	xmlFreeDoc(import);
	}

error:
    if (uriRef != NULL)
	xmlFree(uriRef);
    if (base != NULL)
	xmlFree(base);
    if (URI != NULL)
	xmlFree(URI);

    return (ret);
}