xmlTextReaderConstValue(xmlTextReaderPtr reader) {
    xmlNodePtr node;
    if (reader == NULL)
	return(NULL);
    if (reader->node == NULL)
	return(NULL);
    if (reader->curnode != NULL)
	node = reader->curnode;
    else
	node = reader->node;

    switch (node->type) {
        case XML_NAMESPACE_DECL:
	    return(((xmlNsPtr) node)->href);
        case XML_ATTRIBUTE_NODE:{
	    xmlAttrPtr attr = (xmlAttrPtr) node;

	    if ((attr->children != NULL) &&
	        (attr->children->type == XML_TEXT_NODE) &&
		(attr->children->next == NULL))
		return(attr->children->content);
	    else {
		if (reader->buffer == NULL) {
		    reader->buffer = xmlBufCreateSize(100);
                    if (reader->buffer == NULL) {
                        xmlGenericError(xmlGenericErrorContext,
                                        "xmlTextReaderSetup : malloc failed\n");
                        return (NULL);
                    }
                } else
                    xmlBufEmpty(reader->buffer);
	        xmlBufGetNodeContent(reader->buffer, node);
		return(xmlBufContent(reader->buffer));
	    }
	    break;
	}
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
	    return(node->content);
	default:
	    break;
    }
    return(NULL);
}