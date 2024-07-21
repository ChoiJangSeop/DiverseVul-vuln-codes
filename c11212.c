xmlAddID(xmlValidCtxtPtr ctxt, xmlDocPtr doc, const xmlChar *value,
         xmlAttrPtr attr) {
    xmlIDPtr ret;
    xmlIDTablePtr table;

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
     * Create the ID table if needed.
     */
    table = (xmlIDTablePtr) doc->ids;
    if (table == NULL)  {
        doc->ids = table = xmlHashCreateDict(0, doc->dict);
    }
    if (table == NULL) {
	xmlVErrMemory(ctxt,
		"xmlAddID: Table creation failed!\n");
        return(NULL);
    }

    ret = (xmlIDPtr) xmlMalloc(sizeof(xmlID));
    if (ret == NULL) {
	xmlVErrMemory(ctxt, "malloc failed");
	return(NULL);
    }

    /*
     * fill the structure.
     */
    ret->value = xmlStrdup(value);
    ret->doc = doc;
    if ((ctxt != NULL) && (ctxt->vstateNr != 0)) {
	/*
	 * Operating in streaming mode, attr is gonna disappear
	 */
	if (doc->dict != NULL)
	    ret->name = xmlDictLookup(doc->dict, attr->name, -1);
	else
	    ret->name = xmlStrdup(attr->name);
	ret->attr = NULL;
    } else {
	ret->attr = attr;
	ret->name = NULL;
    }
    ret->lineno = xmlGetLineNo(attr->parent);

    if (xmlHashAddEntry(table, value, ret) < 0) {
#ifdef LIBXML_VALID_ENABLED
	/*
	 * The id is already defined in this DTD.
	 */
	if (ctxt != NULL) {
	    xmlErrValidNode(ctxt, attr->parent, XML_DTD_ID_REDEFINED,
			    "ID %s already defined\n", value, NULL, NULL);
	}
#endif /* LIBXML_VALID_ENABLED */
	xmlFreeID(ret);
	return(NULL);
    }
    if (attr != NULL)
	attr->atype = XML_ATTRIBUTE_ID;
    return(ret);
}