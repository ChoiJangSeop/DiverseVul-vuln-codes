xmlSchemaFormatNodeForError(xmlChar ** msg,
			    xmlSchemaAbstractCtxtPtr actxt,
			    xmlNodePtr node)
{
    xmlChar *str = NULL;

    *msg = NULL;
    if ((node != NULL) &&
	(node->type != XML_ELEMENT_NODE) &&
	(node->type != XML_ATTRIBUTE_NODE))
    {
	/*
	* Don't try to format other nodes than element and
	* attribute nodes.
	* Play safe and return an empty string.
	*/
	*msg = xmlStrdup(BAD_CAST "");
	return(*msg);
    }
    if (node != NULL) {
	/*
	* Work on tree nodes.
	*/
	if (node->type == XML_ATTRIBUTE_NODE) {
	    xmlNodePtr elem = node->parent;

	    *msg = xmlStrdup(BAD_CAST "Element '");
	    if (elem->ns != NULL)
		*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		    elem->ns->href, elem->name));
	    else
		*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		    NULL, elem->name));
	    FREE_AND_NULL(str);
	    *msg = xmlStrcat(*msg, BAD_CAST "', ");
	    *msg = xmlStrcat(*msg, BAD_CAST "attribute '");
	} else {
	    *msg = xmlStrdup(BAD_CAST "Element '");
	}
	if (node->ns != NULL)
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    node->ns->href, node->name));
	else
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    NULL, node->name));
	FREE_AND_NULL(str);
	*msg = xmlStrcat(*msg, BAD_CAST "': ");
    } else if (actxt->type == XML_SCHEMA_CTXT_VALIDATOR) {
	xmlSchemaValidCtxtPtr vctxt = (xmlSchemaValidCtxtPtr) actxt;
	/*
	* Work on node infos.
	*/
	if (vctxt->inode->nodeType == XML_ATTRIBUTE_NODE) {
	    xmlSchemaNodeInfoPtr ielem =
		vctxt->elemInfos[vctxt->depth];

	    *msg = xmlStrdup(BAD_CAST "Element '");
	    *msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
		ielem->nsName, ielem->localName));
	    FREE_AND_NULL(str);
	    *msg = xmlStrcat(*msg, BAD_CAST "', ");
	    *msg = xmlStrcat(*msg, BAD_CAST "attribute '");
	} else {
	    *msg = xmlStrdup(BAD_CAST "Element '");
	}
	*msg = xmlStrcat(*msg, xmlSchemaFormatQName(&str,
	    vctxt->inode->nsName, vctxt->inode->localName));
	FREE_AND_NULL(str);
	*msg = xmlStrcat(*msg, BAD_CAST "': ");
    } else if (actxt->type == XML_SCHEMA_CTXT_PARSER) {
	/*
	* Hmm, no node while parsing?
	* Return an empty string, in case NULL will break something.
	*/
	*msg = xmlStrdup(BAD_CAST "");
    } else {
	TODO
	return (NULL);
    }
    /*
    * VAL TODO: The output of the given schema component is currently
    * disabled.
    */
#if 0
    if ((type != NULL) && (xmlSchemaIsGlobalItem(type))) {
	*msg = xmlStrcat(*msg, BAD_CAST " [");
	*msg = xmlStrcat(*msg, xmlSchemaFormatItemForReport(&str,
	    NULL, type, NULL, 0));
	FREE_AND_NULL(str)
	*msg = xmlStrcat(*msg, BAD_CAST "]");
    }
#endif
    return (*msg);
}