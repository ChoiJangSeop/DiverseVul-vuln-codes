xmlSchemaInternalErr2(xmlSchemaAbstractCtxtPtr actxt,
		     const char *funcName,
		     const char *message,
		     const xmlChar *str1,
		     const xmlChar *str2)
{
    xmlChar *msg = NULL;

    if (actxt == NULL)
        return;
    msg = xmlStrdup(BAD_CAST "Internal error: ");
    msg = xmlStrcat(msg, BAD_CAST funcName);
    msg = xmlStrcat(msg, BAD_CAST ", ");
    msg = xmlStrcat(msg, BAD_CAST message);
    msg = xmlStrcat(msg, BAD_CAST ".\n");

    if (actxt->type == XML_SCHEMA_CTXT_VALIDATOR)
	xmlSchemaErr(actxt, XML_SCHEMAV_INTERNAL, NULL,
	    (const char *) msg, str1, str2);

    else if (actxt->type == XML_SCHEMA_CTXT_PARSER)
	xmlSchemaErr(actxt, XML_SCHEMAP_INTERNAL, NULL,
	    (const char *) msg, str1, str2);

    FREE_AND_NULL(msg)
}