htmlParseChunk(htmlParserCtxtPtr ctxt, const char *chunk, int size,
              int terminate) {
    if ((ctxt == NULL) || (ctxt->input == NULL)) {
	htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
		     "htmlParseChunk: context error\n", NULL, NULL);
	return(XML_ERR_INTERNAL_ERROR);
    }
    if ((size > 0) && (chunk != NULL) && (ctxt->input != NULL) &&
        (ctxt->input->buf != NULL) && (ctxt->instate != XML_PARSER_EOF))  {
	size_t base = xmlBufGetInputBase(ctxt->input->buf->buffer, ctxt->input);
	size_t cur = ctxt->input->cur - ctxt->input->base;
	int res;

	res = xmlParserInputBufferPush(ctxt->input->buf, size, chunk);
	if (res < 0) {
	    ctxt->errNo = XML_PARSER_EOF;
	    ctxt->disableSAX = 1;
	    return (XML_PARSER_EOF);
	}
        xmlBufSetInputBaseCur(ctxt->input->buf->buffer, ctxt->input, base, cur);
#ifdef DEBUG_PUSH
	xmlGenericError(xmlGenericErrorContext, "HPP: pushed %d\n", size);
#endif

#if 0
	if ((terminate) || (ctxt->input->buf->buffer->use > 80))
	    htmlParseTryOrFinish(ctxt, terminate);
#endif
    } else if (ctxt->instate != XML_PARSER_EOF) {
	if ((ctxt->input != NULL) && ctxt->input->buf != NULL) {
	    xmlParserInputBufferPtr in = ctxt->input->buf;
	    if ((in->encoder != NULL) && (in->buffer != NULL) &&
		    (in->raw != NULL)) {
		int nbchars;

		nbchars = xmlCharEncInput(in);
		if (nbchars < 0) {
		    htmlParseErr(ctxt, XML_ERR_INVALID_ENCODING,
			         "encoder error\n", NULL, NULL);
		    return(XML_ERR_INVALID_ENCODING);
		}
	    }
	}
    }
    htmlParseTryOrFinish(ctxt, terminate);
    if (terminate) {
	if ((ctxt->instate != XML_PARSER_EOF) &&
	    (ctxt->instate != XML_PARSER_EPILOG) &&
	    (ctxt->instate != XML_PARSER_MISC)) {
	    ctxt->errNo = XML_ERR_DOCUMENT_END;
	    ctxt->wellFormed = 0;
	}
	if (ctxt->instate != XML_PARSER_EOF) {
	    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		ctxt->sax->endDocument(ctxt->userData);
	}
	ctxt->instate = XML_PARSER_EOF;
    }
    return((xmlParserErrors) ctxt->errNo);
}