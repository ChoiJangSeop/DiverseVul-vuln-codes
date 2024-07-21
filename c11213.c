xmlAddRef(xmlValidCtxtPtr ctxt, xmlDocPtr doc, const xmlChar *value,
    xmlAttrPtr attr) {
    xmlRefPtr ret;
    xmlRefTablePtr table;
    xmlListPtr ref_list;

    if (doc == NULL) {
        return(NULL);
    }
    if (value == NULL) {
        return(NULL);
    }
    if (attr == NULL) {
        return(NULL);
    }

    /*
     * Create the Ref table if needed.
     */
    table = (xmlRefTablePtr) doc->refs;
    if (table == NULL) {
        doc->refs = table = xmlHashCreateDict(0, doc->dict);
    }
    if (table == NULL) {
	xmlVErrMemory(ctxt,
            "xmlAddRef: Table creation failed!\n");
        return(NULL);
    }

    ret = (xmlRefPtr) xmlMalloc(sizeof(xmlRef));
    if (ret == NULL) {
	xmlVErrMemory(ctxt, "malloc failed");
        return(NULL);
    }

    /*
     * fill the structure.
     */
    ret->value = xmlStrdup(value);
    if ((ctxt != NULL) && (ctxt->vstateNr != 0)) {
	/*
	 * Operating in streaming mode, attr is gonna disappear
	 */
	ret->name = xmlStrdup(attr->name);
	ret->attr = NULL;
    } else {
	ret->name = NULL;
	ret->attr = attr;
    }
    ret->lineno = xmlGetLineNo(attr->parent);

    /* To add a reference :-
     * References are maintained as a list of references,
     * Lookup the entry, if no entry create new nodelist
     * Add the owning node to the NodeList
     * Return the ref
     */

    if (NULL == (ref_list = xmlHashLookup(table, value))) {
        if (NULL == (ref_list = xmlListCreate(xmlFreeRef, xmlDummyCompare))) {
	    xmlErrValid(NULL, XML_ERR_INTERNAL_ERROR,
		    "xmlAddRef: Reference list creation failed!\n",
		    NULL);
	    goto failed;
        }
        if (xmlHashAddEntry(table, value, ref_list) < 0) {
            xmlListDelete(ref_list);
	    xmlErrValid(NULL, XML_ERR_INTERNAL_ERROR,
		    "xmlAddRef: Reference list insertion failed!\n",
		    NULL);
	    goto failed;
        }
    }
    if (xmlListAppend(ref_list, ret) != 0) {
	xmlErrValid(NULL, XML_ERR_INTERNAL_ERROR,
		    "xmlAddRef: Reference list insertion failed!\n",
		    NULL);
        goto failed;
    }
    return(ret);
failed:
    if (ret != NULL) {
        if (ret->value != NULL)
	    xmlFree((char *)ret->value);
        if (ret->name != NULL)
	    xmlFree((char *)ret->name);
        xmlFree(ret);
    }
    return(NULL);
}