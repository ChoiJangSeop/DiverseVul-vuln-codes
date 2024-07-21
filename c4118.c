xmlXPtrNewContext(xmlDocPtr doc, xmlNodePtr here, xmlNodePtr origin) {
    xmlXPathContextPtr ret;

    ret = xmlXPathNewContext(doc);
    if (ret == NULL)
	return(ret);
    ret->xptr = 1;
    ret->here = here;
    ret->origin = origin;

    xmlXPathRegisterFunc(ret, (xmlChar *)"range-to",
	                 xmlXPtrRangeToFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"range",
	                 xmlXPtrRangeFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"range-inside",
	                 xmlXPtrRangeInsideFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"string-range",
	                 xmlXPtrStringRangeFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"start-point",
	                 xmlXPtrStartPointFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"end-point",
	                 xmlXPtrEndPointFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)"here",
	                 xmlXPtrHereFunction);
    xmlXPathRegisterFunc(ret, (xmlChar *)" origin",
	                 xmlXPtrOriginFunction);

    return(ret);
}