xmlTextReaderSetup(xmlTextReaderPtr reader,
                   xmlParserInputBufferPtr input, const char *URL,
                   const char *encoding, int options)
{
    if (reader == NULL) {
        if (input != NULL)
	    xmlFreeParserInputBuffer(input);
        return (-1);
    }

    /*
     * we force the generation of compact text nodes on the reader
     * since usr applications should never modify the tree
     */
    options |= XML_PARSE_COMPACT;

    reader->doc = NULL;
    reader->entNr = 0;
    reader->parserFlags = options;
    reader->validate = XML_TEXTREADER_NOT_VALIDATE;
    if ((input != NULL) && (reader->input != NULL) &&
        (reader->allocs & XML_TEXTREADER_INPUT)) {
	xmlFreeParserInputBuffer(reader->input);
	reader->input = NULL;
	reader->allocs -= XML_TEXTREADER_INPUT;
    }
    if (input != NULL) {
	reader->input = input;
	reader->allocs |= XML_TEXTREADER_INPUT;
    }
    if (reader->buffer == NULL)
        reader->buffer = xmlBufCreateSize(100);
    if (reader->buffer == NULL) {
        xmlGenericError(xmlGenericErrorContext,
                        "xmlTextReaderSetup : malloc failed\n");
        return (-1);
    }
    if (reader->sax == NULL)
	reader->sax = (xmlSAXHandler *) xmlMalloc(sizeof(xmlSAXHandler));
    if (reader->sax == NULL) {
        xmlGenericError(xmlGenericErrorContext,
                        "xmlTextReaderSetup : malloc failed\n");
        return (-1);
    }
    xmlSAXVersion(reader->sax, 2);
    reader->startElement = reader->sax->startElement;
    reader->sax->startElement = xmlTextReaderStartElement;
    reader->endElement = reader->sax->endElement;
    reader->sax->endElement = xmlTextReaderEndElement;
#ifdef LIBXML_SAX1_ENABLED
    if (reader->sax->initialized == XML_SAX2_MAGIC) {
#endif /* LIBXML_SAX1_ENABLED */
        reader->startElementNs = reader->sax->startElementNs;
        reader->sax->startElementNs = xmlTextReaderStartElementNs;
        reader->endElementNs = reader->sax->endElementNs;
        reader->sax->endElementNs = xmlTextReaderEndElementNs;
#ifdef LIBXML_SAX1_ENABLED
    } else {
        reader->startElementNs = NULL;
        reader->endElementNs = NULL;
    }
#endif /* LIBXML_SAX1_ENABLED */
    reader->characters = reader->sax->characters;
    reader->sax->characters = xmlTextReaderCharacters;
    reader->sax->ignorableWhitespace = xmlTextReaderCharacters;
    reader->cdataBlock = reader->sax->cdataBlock;
    reader->sax->cdataBlock = xmlTextReaderCDataBlock;

    reader->mode = XML_TEXTREADER_MODE_INITIAL;
    reader->node = NULL;
    reader->curnode = NULL;
    if (input != NULL) {
        if (xmlBufUse(reader->input->buffer) < 4) {
            xmlParserInputBufferRead(input, 4);
        }
        if (reader->ctxt == NULL) {
            if (xmlBufUse(reader->input->buffer) >= 4) {
                reader->ctxt = xmlCreatePushParserCtxt(reader->sax, NULL,
		       (const char *) xmlBufContent(reader->input->buffer),
                                      4, URL);
                reader->base = 0;
                reader->cur = 4;
            } else {
                reader->ctxt =
                    xmlCreatePushParserCtxt(reader->sax, NULL, NULL, 0, URL);
                reader->base = 0;
                reader->cur = 0;
            }
        } else {
	    xmlParserInputPtr inputStream;
	    xmlParserInputBufferPtr buf;
	    xmlCharEncoding enc = XML_CHAR_ENCODING_NONE;

	    xmlCtxtReset(reader->ctxt);
	    buf = xmlAllocParserInputBuffer(enc);
	    if (buf == NULL) return(-1);
	    inputStream = xmlNewInputStream(reader->ctxt);
	    if (inputStream == NULL) {
		xmlFreeParserInputBuffer(buf);
		return(-1);
	    }

	    if (URL == NULL)
		inputStream->filename = NULL;
	    else
		inputStream->filename = (char *)
		    xmlCanonicPath((const xmlChar *) URL);
	    inputStream->buf = buf;
            xmlBufResetInput(buf->buffer, inputStream);

	    inputPush(reader->ctxt, inputStream);
	    reader->cur = 0;
	}
        if (reader->ctxt == NULL) {
            xmlGenericError(xmlGenericErrorContext,
                            "xmlTextReaderSetup : malloc failed\n");
            return (-1);
        }
    }
    if (reader->dict != NULL) {
        if (reader->ctxt->dict != NULL) {
	    if (reader->dict != reader->ctxt->dict) {
		xmlDictFree(reader->dict);
		reader->dict = reader->ctxt->dict;
	    }
	} else {
	    reader->ctxt->dict = reader->dict;
	}
    } else {
	if (reader->ctxt->dict == NULL)
	    reader->ctxt->dict = xmlDictCreate();
        reader->dict = reader->ctxt->dict;
    }
    reader->ctxt->_private = reader;
    reader->ctxt->linenumbers = 1;
    reader->ctxt->dictNames = 1;
    /*
     * use the parser dictionnary to allocate all elements and attributes names
     */
    reader->ctxt->docdict = 1;
    reader->ctxt->parseMode = XML_PARSE_READER;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (reader->xincctxt != NULL) {
	xmlXIncludeFreeContext(reader->xincctxt);
	reader->xincctxt = NULL;
    }
    if (options & XML_PARSE_XINCLUDE) {
        reader->xinclude = 1;
	reader->xinclude_name = xmlDictLookup(reader->dict, XINCLUDE_NODE, -1);
	options -= XML_PARSE_XINCLUDE;
    } else
        reader->xinclude = 0;
    reader->in_xinclude = 0;
#endif
#ifdef LIBXML_PATTERN_ENABLED
    if (reader->patternTab == NULL) {
        reader->patternNr = 0;
	reader->patternMax = 0;
    }
    while (reader->patternNr > 0) {
        reader->patternNr--;
	if (reader->patternTab[reader->patternNr] != NULL) {
	    xmlFreePattern(reader->patternTab[reader->patternNr]);
            reader->patternTab[reader->patternNr] = NULL;
	}
    }
#endif

    if (options & XML_PARSE_DTDVALID)
        reader->validate = XML_TEXTREADER_VALIDATE_DTD;

    xmlCtxtUseOptions(reader->ctxt, options);
    if (encoding != NULL) {
        xmlCharEncodingHandlerPtr hdlr;

        hdlr = xmlFindCharEncodingHandler(encoding);
        if (hdlr != NULL)
            xmlSwitchToEncoding(reader->ctxt, hdlr);
    }
    if ((URL != NULL) && (reader->ctxt->input != NULL) &&
        (reader->ctxt->input->filename == NULL))
        reader->ctxt->input->filename = (char *)
            xmlStrdup((const xmlChar *) URL);

    reader->doc = NULL;

    return (0);
}