xmlValidCtxtNormalizeAttributeValue(xmlValidCtxtPtr ctxt, xmlDocPtr doc,
	     xmlNodePtr elem, const xmlChar *name, const xmlChar *value) {
    xmlChar *ret, *dst;
    const xmlChar *src;
    xmlAttributePtr attrDecl = NULL;
    int extsubset = 0;

    if (doc == NULL) return(NULL);
    if (elem == NULL) return(NULL);
    if (name == NULL) return(NULL);
    if (value == NULL) return(NULL);

    if ((elem->ns != NULL) && (elem->ns->prefix != NULL)) {
	xmlChar fn[50];
	xmlChar *fullname;

	fullname = xmlBuildQName(elem->name, elem->ns->prefix, fn, 50);
	if (fullname == NULL)
	    return(NULL);
	attrDecl = xmlGetDtdAttrDesc(doc->intSubset, fullname, name);
	if ((attrDecl == NULL) && (doc->extSubset != NULL)) {
	    attrDecl = xmlGetDtdAttrDesc(doc->extSubset, fullname, name);
	    if (attrDecl != NULL)
		extsubset = 1;
	}
	if ((fullname != fn) && (fullname != elem->name))
	    xmlFree(fullname);
    }
    if ((attrDecl == NULL) && (doc->intSubset != NULL))
	attrDecl = xmlGetDtdAttrDesc(doc->intSubset, elem->name, name);
    if ((attrDecl == NULL) && (doc->extSubset != NULL)) {
	attrDecl = xmlGetDtdAttrDesc(doc->extSubset, elem->name, name);
	if (attrDecl != NULL)
	    extsubset = 1;
    }

    if (attrDecl == NULL)
	return(NULL);
    if (attrDecl->atype == XML_ATTRIBUTE_CDATA)
	return(NULL);

    ret = xmlStrdup(value);
    if (ret == NULL)
	return(NULL);
    src = value;
    dst = ret;
    while (*src == 0x20) src++;
    while (*src != 0) {
	if (*src == 0x20) {
	    while (*src == 0x20) src++;
	    if (*src != 0)
		*dst++ = 0x20;
	} else {
	    *dst++ = *src++;
	}
    }
    *dst = 0;
    if ((doc->standalone) && (extsubset == 1) && (!xmlStrEqual(value, ret))) {
	xmlErrValidNode(ctxt, elem, XML_DTD_NOT_STANDALONE,
"standalone: %s on %s value had to be normalized based on external subset declaration\n",
	       name, elem->name, NULL);
	ctxt->valid = 0;
    }
    return(ret);
}