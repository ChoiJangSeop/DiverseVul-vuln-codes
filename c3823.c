xmlSchemaPSimpleTypeErr(xmlSchemaParserCtxtPtr ctxt,
			xmlParserErrors error,
			xmlSchemaBasicItemPtr ownerItem ATTRIBUTE_UNUSED,
			xmlNodePtr node,
			xmlSchemaTypePtr type,
			const char *expected,
			const xmlChar *value,
			const char *message,
			const xmlChar *str1,
			const xmlChar *str2)
{
    xmlChar *msg = NULL;

    xmlSchemaFormatNodeForError(&msg, ACTXT_CAST ctxt, node);
    if (message == NULL) {
	/*
	* Use default messages.
	*/
	if (type != NULL) {
	    if (node->type == XML_ATTRIBUTE_NODE)
		msg = xmlStrcat(msg, BAD_CAST "'%s' is not a valid value of ");
	    else
		msg = xmlStrcat(msg, BAD_CAST "The character content is not a "
		"valid value of ");
	    if (! xmlSchemaIsGlobalItem(type))
		msg = xmlStrcat(msg, BAD_CAST "the local ");
	    else
		msg = xmlStrcat(msg, BAD_CAST "the ");

	    if (WXS_IS_ATOMIC(type))
		msg = xmlStrcat(msg, BAD_CAST "atomic type");
	    else if (WXS_IS_LIST(type))
		msg = xmlStrcat(msg, BAD_CAST "list type");
	    else if (WXS_IS_UNION(type))
		msg = xmlStrcat(msg, BAD_CAST "union type");

	    if (xmlSchemaIsGlobalItem(type)) {
		xmlChar *str = NULL;
		msg = xmlStrcat(msg, BAD_CAST " '");
		if (type->builtInType != 0) {
		    msg = xmlStrcat(msg, BAD_CAST "xs:");
		    msg = xmlStrcat(msg, type->name);
		} else
		    msg = xmlStrcat(msg,
			xmlSchemaFormatQName(&str,
			    type->targetNamespace, type->name));
		msg = xmlStrcat(msg, BAD_CAST "'.");
		FREE_AND_NULL(str);
	    }
	} else {
	    if (node->type == XML_ATTRIBUTE_NODE)
		msg = xmlStrcat(msg, BAD_CAST "The value '%s' is not valid.");
	    else
		msg = xmlStrcat(msg, BAD_CAST "The character content is not "
		"valid.");
	}
	if (expected) {
	    msg = xmlStrcat(msg, BAD_CAST " Expected is '");
	    msg = xmlStrcat(msg, BAD_CAST expected);
	    msg = xmlStrcat(msg, BAD_CAST "'.\n");
	} else
	    msg = xmlStrcat(msg, BAD_CAST "\n");
	if (node->type == XML_ATTRIBUTE_NODE)
	    xmlSchemaPErr(ctxt, node, error, (const char *) msg, value, NULL);
	else
	    xmlSchemaPErr(ctxt, node, error, (const char *) msg, NULL, NULL);
    } else {
	msg = xmlStrcat(msg, BAD_CAST message);
	msg = xmlStrcat(msg, BAD_CAST ".\n");
	xmlSchemaPErrExt(ctxt, node, error, NULL, NULL, NULL,
	     (const char*) msg, str1, str2, NULL, NULL, NULL);
    }
    /* Cleanup. */
    FREE_AND_NULL(msg)
}