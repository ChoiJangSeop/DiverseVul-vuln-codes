xmlNewTextReader(xmlParserInputBufferPtr input, const char *URI) {
    xmlTextReaderPtr ret;

    if (input == NULL)
	return(NULL);
    ret = xmlMalloc(sizeof(xmlTextReader));
    if (ret == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xmlNewTextReader : malloc failed\n");
	return(NULL);
    }
    memset(ret, 0, sizeof(xmlTextReader));
    ret->doc = NULL;
    ret->entTab = NULL;
    ret->entMax = 0;
    ret->entNr = 0;
    ret->input = input;
    ret->buffer = xmlBufCreateSize(100);
    if (ret->buffer == NULL) {
        xmlFree(ret);
        xmlGenericError(xmlGenericErrorContext,
		"xmlNewTextReader : malloc failed\n");
	return(NULL);
    }
    ret->sax = (xmlSAXHandler *) xmlMalloc(sizeof(xmlSAXHandler));
    if (ret->sax == NULL) {
	xmlBufFree(ret->buffer);
	xmlFree(ret);
        xmlGenericError(xmlGenericErrorContext,
		"xmlNewTextReader : malloc failed\n");
	return(NULL);
    }
    xmlSAXVersion(ret->sax, 2);
    ret->startElement = ret->sax->startElement;
    ret->sax->startElement = xmlTextReaderStartElement;
    ret->endElement = ret->sax->endElement;
    ret->sax->endElement = xmlTextReaderEndElement;
#ifdef LIBXML_SAX1_ENABLED
    if (ret->sax->initialized == XML_SAX2_MAGIC) {
#endif /* LIBXML_SAX1_ENABLED */
	ret->startElementNs = ret->sax->startElementNs;
	ret->sax->startElementNs = xmlTextReaderStartElementNs;
	ret->endElementNs = ret->sax->endElementNs;
	ret->sax->endElementNs = xmlTextReaderEndElementNs;
#ifdef LIBXML_SAX1_ENABLED
    } else {
	ret->startElementNs = NULL;
	ret->endElementNs = NULL;
    }
#endif /* LIBXML_SAX1_ENABLED */
    ret->characters = ret->sax->characters;
    ret->sax->characters = xmlTextReaderCharacters;
    ret->sax->ignorableWhitespace = xmlTextReaderCharacters;
    ret->cdataBlock = ret->sax->cdataBlock;
    ret->sax->cdataBlock = xmlTextReaderCDataBlock;

    ret->mode = XML_TEXTREADER_MODE_INITIAL;
    ret->node = NULL;
    ret->curnode = NULL;
    if (xmlBufUse(ret->input->buffer) < 4) {
	xmlParserInputBufferRead(input, 4);
    }
    if (xmlBufUse(ret->input->buffer) >= 4) {
	ret->ctxt = xmlCreatePushParserCtxt(ret->sax, NULL,
			     (const char *) xmlBufContent(ret->input->buffer),
                                            4, URI);
	ret->base = 0;
	ret->cur = 4;
    } else {
	ret->ctxt = xmlCreatePushParserCtxt(ret->sax, NULL, NULL, 0, URI);
	ret->base = 0;
	ret->cur = 0;
    }

    if (ret->ctxt == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xmlNewTextReader : malloc failed\n");
	xmlBufFree(ret->buffer);
	xmlFree(ret->sax);
	xmlFree(ret);
	return(NULL);
    }
    ret->ctxt->parseMode = XML_PARSE_READER;
    ret->ctxt->_private = ret;
    ret->ctxt->linenumbers = 1;
    ret->ctxt->dictNames = 1;
    ret->allocs = XML_TEXTREADER_CTXT;
    /*
     * use the parser dictionnary to allocate all elements and attributes names
     */
    ret->ctxt->docdict = 1;
    ret->dict = ret->ctxt->dict;
#ifdef LIBXML_XINCLUDE_ENABLED
    ret->xinclude = 0;
#endif
#ifdef LIBXML_PATTERN_ENABLED
    ret->patternMax = 0;
    ret->patternTab = NULL;
#endif
    return(ret);
}