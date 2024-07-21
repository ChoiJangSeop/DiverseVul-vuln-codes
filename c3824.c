xmlSchemaSimpleTypeErr(xmlSchemaAbstractCtxtPtr actxt,
		       xmlParserErrors error,
		       xmlNodePtr node,
		       const xmlChar *value,
		       xmlSchemaTypePtr type,
		       int displayValue)
{
    xmlChar *msg = NULL;

    xmlSchemaFormatNodeForError(&msg, actxt, node);

    if (displayValue || (xmlSchemaEvalErrorNodeType(actxt, node) ==
	    XML_ATTRIBUTE_NODE))
	msg = xmlStrcat(msg, BAD_CAST "'%s' is not a valid value of ");
    else
	msg = xmlStrcat(msg, BAD_CAST "The character content is not a valid "
	    "value of ");

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
	msg = xmlStrcat(msg, BAD_CAST "'");
	FREE_AND_NULL(str);
    }
    msg = xmlStrcat(msg, BAD_CAST ".\n");
    if (displayValue || (xmlSchemaEvalErrorNodeType(actxt, node) ==
	    XML_ATTRIBUTE_NODE))
	xmlSchemaErr(actxt, error, node, (const char *) msg, value, NULL);
    else
	xmlSchemaErr(actxt, error, node, (const char *) msg, NULL, NULL);
    FREE_AND_NULL(msg)
}