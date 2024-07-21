htmlParseTryOrFinish(htmlParserCtxtPtr ctxt, int terminate) {
    int ret = 0;
    htmlParserInputPtr in;
    int avail = 0;
    xmlChar cur, next;

    htmlParserNodeInfo node_info;

#ifdef DEBUG_PUSH
    switch (ctxt->instate) {
	case XML_PARSER_EOF:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try EOF\n"); break;
	case XML_PARSER_START:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try START\n"); break;
	case XML_PARSER_MISC:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try MISC\n");break;
	case XML_PARSER_COMMENT:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try COMMENT\n");break;
	case XML_PARSER_PROLOG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try PROLOG\n");break;
	case XML_PARSER_START_TAG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try START_TAG\n");break;
	case XML_PARSER_CONTENT:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try CONTENT\n");break;
	case XML_PARSER_CDATA_SECTION:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try CDATA_SECTION\n");break;
	case XML_PARSER_END_TAG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try END_TAG\n");break;
	case XML_PARSER_ENTITY_DECL:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ENTITY_DECL\n");break;
	case XML_PARSER_ENTITY_VALUE:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ENTITY_VALUE\n");break;
	case XML_PARSER_ATTRIBUTE_VALUE:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try ATTRIBUTE_VALUE\n");break;
	case XML_PARSER_DTD:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try DTD\n");break;
	case XML_PARSER_EPILOG:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try EPILOG\n");break;
	case XML_PARSER_PI:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try PI\n");break;
	case XML_PARSER_SYSTEM_LITERAL:
	    xmlGenericError(xmlGenericErrorContext,
		    "HPP: try SYSTEM_LITERAL\n");break;
    }
#endif

    while (1) {

	in = ctxt->input;
	if (in == NULL) break;
	if (in->buf == NULL)
	    avail = in->length - (in->cur - in->base);
	else
	    avail = xmlBufUse(in->buf->buffer) - (in->cur - in->base);
	if ((avail == 0) && (terminate)) {
	    htmlAutoCloseOnEnd(ctxt);
	    if ((ctxt->nameNr == 0) && (ctxt->instate != XML_PARSER_EOF)) {
		/*
		 * SAX: end of the document processing.
		 */
		ctxt->instate = XML_PARSER_EOF;
		if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		    ctxt->sax->endDocument(ctxt->userData);
	    }
	}
        if (avail < 1)
	    goto done;
	cur = in->cur[0];
	if (cur == 0) {
	    SKIP(1);
	    continue;
	}

        switch (ctxt->instate) {
            case XML_PARSER_EOF:
	        /*
		 * Document parsing is done !
		 */
	        goto done;
            case XML_PARSER_START:
	        /*
		 * Very first chars read from the document flow.
		 */
		cur = in->cur[0];
		if (IS_BLANK_CH(cur)) {
		    SKIP_BLANKS;
		    if (in->buf == NULL)
			avail = in->length - (in->cur - in->base);
		    else
			avail = xmlBufUse(in->buf->buffer) - (in->cur - in->base);
		}
		if ((ctxt->sax) && (ctxt->sax->setDocumentLocator))
		    ctxt->sax->setDocumentLocator(ctxt->userData,
						  &xmlDefaultSAXLocator);
		if ((ctxt->sax) && (ctxt->sax->startDocument) &&
	            (!ctxt->disableSAX))
		    ctxt->sax->startDocument(ctxt->userData);

		cur = in->cur[0];
		next = in->cur[1];
		if ((cur == '<') && (next == '!') &&
		    (UPP(2) == 'D') && (UPP(3) == 'O') &&
		    (UPP(4) == 'C') && (UPP(5) == 'T') &&
		    (UPP(6) == 'Y') && (UPP(7) == 'P') &&
		    (UPP(8) == 'E')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing internal subset\n");
#endif
		    htmlParseDocTypeDecl(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering PROLOG\n");
#endif
                } else {
		    ctxt->instate = XML_PARSER_MISC;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering MISC\n");
#endif
		}
		break;
            case XML_PARSER_MISC:
		SKIP_BLANKS;
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = xmlBufUse(in->buf->buffer) - (in->cur - in->base);
		/*
		 * no chars in buffer
		 */
		if (avail < 1)
		    goto done;
		/*
		 * not enouth chars in buffer
		 */
		if (avail < 2) {
		    if (!terminate)
			goto done;
		    else
			next = ' ';
		} else {
		    next = in->cur[1];
		}
		cur = in->cur[0];
	        if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>', 1, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_MISC;
	        } else if ((cur == '<') && (next == '?')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing PI\n");
#endif
		    htmlParsePI(ctxt);
		    ctxt->instate = XML_PARSER_MISC;
		} else if ((cur == '<') && (next == '!') &&
		    (UPP(2) == 'D') && (UPP(3) == 'O') &&
		    (UPP(4) == 'C') && (UPP(5) == 'T') &&
		    (UPP(6) == 'Y') && (UPP(7) == 'P') &&
		    (UPP(8) == 'E')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing internal subset\n");
#endif
		    htmlParseDocTypeDecl(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering PROLOG\n");
#endif
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 9)) {
		    goto done;
		} else {
		    ctxt->instate = XML_PARSER_START_TAG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering START_TAG\n");
#endif
		}
		break;
            case XML_PARSER_PROLOG:
		SKIP_BLANKS;
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = xmlBufUse(in->buf->buffer) - (in->cur - in->base);
		if (avail < 2)
		    goto done;
		cur = in->cur[0];
		next = in->cur[1];
		if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>', 1, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
	        } else if ((cur == '<') && (next == '?')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing PI\n");
#endif
		    htmlParsePI(ctxt);
		    ctxt->instate = XML_PARSER_PROLOG;
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 4)) {
		    goto done;
		} else {
		    ctxt->instate = XML_PARSER_START_TAG;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering START_TAG\n");
#endif
		}
		break;
            case XML_PARSER_EPILOG:
		if (in->buf == NULL)
		    avail = in->length - (in->cur - in->base);
		else
		    avail = xmlBufUse(in->buf->buffer) - (in->cur - in->base);
		if (avail < 1)
		    goto done;
		cur = in->cur[0];
		if (IS_BLANK_CH(cur)) {
		    htmlParseCharData(ctxt);
		    goto done;
		}
		if (avail < 2)
		    goto done;
		next = in->cur[1];
	        if ((cur == '<') && (next == '!') &&
		    (in->cur[2] == '-') && (in->cur[3] == '-')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '-', '-', '>', 1, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing Comment\n");
#endif
		    htmlParseComment(ctxt);
		    ctxt->instate = XML_PARSER_EPILOG;
	        } else if ((cur == '<') && (next == '?')) {
		    if ((!terminate) &&
		        (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			goto done;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: Parsing PI\n");
#endif
		    htmlParsePI(ctxt);
		    ctxt->instate = XML_PARSER_EPILOG;
		} else if ((cur == '<') && (next == '!') &&
		           (avail < 4)) {
		    goto done;
		} else {
		    ctxt->errNo = XML_ERR_DOCUMENT_END;
		    ctxt->wellFormed = 0;
		    ctxt->instate = XML_PARSER_EOF;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering EOF\n");
#endif
		    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
			ctxt->sax->endDocument(ctxt->userData);
		    goto done;
		}
		break;
            case XML_PARSER_START_TAG: {
	        const xmlChar *name;
		int failed;
		const htmlElemDesc * info;

		/*
		 * no chars in buffer
		 */
		if (avail < 1)
		    goto done;
		/*
		 * not enouth chars in buffer
		 */
		if (avail < 2) {
		    if (!terminate)
			goto done;
		    else
			next = ' ';
		} else {
		    next = in->cur[1];
		}
		cur = in->cur[0];
	        if (cur != '<') {
		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}
		if (next == '/') {
		    ctxt->instate = XML_PARSER_END_TAG;
		    ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering END_TAG\n");
#endif
		    break;
		}
		if ((!terminate) &&
		    (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
		    goto done;

                /* Capture start position */
	        if (ctxt->record_info) {
	             node_info.begin_pos = ctxt->input->consumed +
	                                (CUR_PTR - ctxt->input->base);
	             node_info.begin_line = ctxt->input->line;
	        }


		failed = htmlParseStartTag(ctxt);
		name = ctxt->name;
		if ((failed == -1) ||
		    (name == NULL)) {
		    if (CUR == '>')
			NEXT;
		    break;
		}

		/*
		 * Lookup the info for that element.
		 */
		info = htmlTagLookup(name);
		if (info == NULL) {
		    htmlParseErr(ctxt, XML_HTML_UNKNOWN_TAG,
		                 "Tag %s invalid\n", name, NULL);
		}

		/*
		 * Check for an Empty Element labeled the XML/SGML way
		 */
		if ((CUR == '/') && (NXT(1) == '>')) {
		    SKIP(2);
		    if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
			ctxt->sax->endElement(ctxt->userData, name);
		    htmlnamePop(ctxt);
		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}

		if (CUR == '>') {
		    NEXT;
		} else {
		    htmlParseErr(ctxt, XML_ERR_GT_REQUIRED,
		                 "Couldn't find end of Start Tag %s\n",
				 name, NULL);

		    /*
		     * end of parsing of this node.
		     */
		    if (xmlStrEqual(name, ctxt->name)) {
			nodePop(ctxt);
			htmlnamePop(ctxt);
		    }

		    if (ctxt->record_info)
		        htmlNodeInfoPush(ctxt, &node_info);

		    ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		    xmlGenericError(xmlGenericErrorContext,
			    "HPP: entering CONTENT\n");
#endif
		    break;
		}

		/*
		 * Check for an Empty Element from DTD definition
		 */
		if ((info != NULL) && (info->empty)) {
		    if ((ctxt->sax != NULL) && (ctxt->sax->endElement != NULL))
			ctxt->sax->endElement(ctxt->userData, name);
		    htmlnamePop(ctxt);
		}

                if (ctxt->record_info)
	            htmlNodeInfoPush(ctxt, &node_info);

		ctxt->instate = XML_PARSER_CONTENT;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
                break;
	    }
            case XML_PARSER_CONTENT: {
		long cons;
                /*
		 * Handle preparsed entities and charRef
		 */
		if (ctxt->token != 0) {
		    xmlChar chr[2] = { 0 , 0 } ;

		    chr[0] = (xmlChar) ctxt->token;
		    htmlCheckParagraph(ctxt);
		    if ((ctxt->sax != NULL) && (ctxt->sax->characters != NULL))
			ctxt->sax->characters(ctxt->userData, chr, 1);
		    ctxt->token = 0;
		    ctxt->checkIndex = 0;
		}
		if ((avail == 1) && (terminate)) {
		    cur = in->cur[0];
		    if ((cur != '<') && (cur != '&')) {
			if (ctxt->sax != NULL) {
			    if (IS_BLANK_CH(cur)) {
				if (ctxt->keepBlanks) {
				    if (ctxt->sax->characters != NULL)
					ctxt->sax->characters(
						ctxt->userData, &cur, 1);
				} else {
				    if (ctxt->sax->ignorableWhitespace != NULL)
					ctxt->sax->ignorableWhitespace(
						ctxt->userData, &cur, 1);
				}
			    } else {
				htmlCheckParagraph(ctxt);
				if (ctxt->sax->characters != NULL)
				    ctxt->sax->characters(
					    ctxt->userData, &cur, 1);
			    }
			}
			ctxt->token = 0;
			ctxt->checkIndex = 0;
			in->cur++;
			break;
		    }
		}
		if (avail < 2)
		    goto done;
		cur = in->cur[0];
		next = in->cur[1];
		cons = ctxt->nbChars;
		if ((xmlStrEqual(ctxt->name, BAD_CAST"script")) ||
		    (xmlStrEqual(ctxt->name, BAD_CAST"style"))) {
		    /*
		     * Handle SCRIPT/STYLE separately
		     */
		    if (!terminate) {
		        int idx;
			xmlChar val;

			idx = htmlParseLookupSequence(ctxt, '<', '/', 0, 0, 0);
			if (idx < 0)
			    goto done;
		        val = in->cur[idx + 2];
			if (val == 0) /* bad cut of input */
			    goto done;
		    }
		    htmlParseScript(ctxt);
		    if ((cur == '<') && (next == '/')) {
			ctxt->instate = XML_PARSER_END_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering END_TAG\n");
#endif
			break;
		    }
		} else {
		    /*
		     * Sometimes DOCTYPE arrives in the middle of the document
		     */
		    if ((cur == '<') && (next == '!') &&
			(UPP(2) == 'D') && (UPP(3) == 'O') &&
			(UPP(4) == 'C') && (UPP(5) == 'T') &&
			(UPP(6) == 'Y') && (UPP(7) == 'P') &&
			(UPP(8) == 'E')) {
			if ((!terminate) &&
			    (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			    goto done;
			htmlParseErr(ctxt, XML_HTML_STRUCURE_ERROR,
			             "Misplaced DOCTYPE declaration\n",
				     BAD_CAST "DOCTYPE" , NULL);
			htmlParseDocTypeDecl(ctxt);
		    } else if ((cur == '<') && (next == '!') &&
			(in->cur[2] == '-') && (in->cur[3] == '-')) {
			if ((!terminate) &&
			    (htmlParseLookupSequence(
				ctxt, '-', '-', '>', 1, 1) < 0))
			    goto done;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing Comment\n");
#endif
			htmlParseComment(ctxt);
			ctxt->instate = XML_PARSER_CONTENT;
		    } else if ((cur == '<') && (next == '?')) {
			if ((!terminate) &&
			    (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
			    goto done;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing PI\n");
#endif
			htmlParsePI(ctxt);
			ctxt->instate = XML_PARSER_CONTENT;
		    } else if ((cur == '<') && (next == '!') && (avail < 4)) {
			goto done;
		    } else if ((cur == '<') && (next == '/')) {
			ctxt->instate = XML_PARSER_END_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering END_TAG\n");
#endif
			break;
		    } else if (cur == '<') {
			ctxt->instate = XML_PARSER_START_TAG;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: entering START_TAG\n");
#endif
			break;
		    } else if (cur == '&') {
			if ((!terminate) &&
			    (htmlParseLookupChars(ctxt,
                                                  BAD_CAST "; >/", 4) < 0))
			    goto done;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing Reference\n");
#endif
			/* TODO: check generation of subtrees if noent !!! */
			htmlParseReference(ctxt);
		    } else {
		        /*
			 * check that the text sequence is complete
			 * before handing out the data to the parser
			 * to avoid problems with erroneous end of
			 * data detection.
			 */
			if ((!terminate) &&
                            (htmlParseLookupChars(ctxt, BAD_CAST "<&", 2) < 0))
			    goto done;
			ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
			xmlGenericError(xmlGenericErrorContext,
				"HPP: Parsing char data\n");
#endif
			htmlParseCharData(ctxt);
		    }
		}
		if (cons == ctxt->nbChars) {
		    if (ctxt->node != NULL) {
			htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			             "detected an error in element content\n",
				     NULL, NULL);
		    }
		    NEXT;
		    break;
		}

		break;
	    }
            case XML_PARSER_END_TAG:
		if (avail < 2)
		    goto done;
		if ((!terminate) &&
		    (htmlParseLookupSequence(ctxt, '>', 0, 0, 0, 1) < 0))
		    goto done;
		htmlParseEndTag(ctxt);
		if (ctxt->nameNr == 0) {
		    ctxt->instate = XML_PARSER_EPILOG;
		} else {
		    ctxt->instate = XML_PARSER_CONTENT;
		}
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
	        break;
            case XML_PARSER_CDATA_SECTION:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == CDATA\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_DTD:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == DTD\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_COMMENT:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == COMMENT\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_PI:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == PI\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_ENTITY_DECL:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == ENTITY_DECL\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
            case XML_PARSER_ENTITY_VALUE:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == ENTITY_VALUE\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering DTD\n");
#endif
		break;
            case XML_PARSER_ATTRIBUTE_VALUE:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == ATTRIBUTE_VALUE\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_START_TAG;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering START_TAG\n");
#endif
		break;
	    case XML_PARSER_SYSTEM_LITERAL:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
		    "HPP: internal error, state == XML_PARSER_SYSTEM_LITERAL\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
	    case XML_PARSER_IGNORE:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == XML_PARSER_IGNORE\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;
	    case XML_PARSER_PUBLIC_LITERAL:
		htmlParseErr(ctxt, XML_ERR_INTERNAL_ERROR,
			"HPP: internal error, state == XML_PARSER_LITERAL\n",
			     NULL, NULL);
		ctxt->instate = XML_PARSER_CONTENT;
		ctxt->checkIndex = 0;
#ifdef DEBUG_PUSH
		xmlGenericError(xmlGenericErrorContext,
			"HPP: entering CONTENT\n");
#endif
		break;

	}
    }
done:
    if ((avail == 0) && (terminate)) {
	htmlAutoCloseOnEnd(ctxt);
	if ((ctxt->nameNr == 0) && (ctxt->instate != XML_PARSER_EOF)) {
	    /*
	     * SAX: end of the document processing.
	     */
	    ctxt->instate = XML_PARSER_EOF;
	    if ((ctxt->sax) && (ctxt->sax->endDocument != NULL))
		ctxt->sax->endDocument(ctxt->userData);
	}
    }
    if ((!(ctxt->options & HTML_PARSE_NODEFDTD)) && (ctxt->myDoc != NULL) &&
	((terminate) || (ctxt->instate == XML_PARSER_EOF) ||
	 (ctxt->instate == XML_PARSER_EPILOG))) {
	xmlDtdPtr dtd;
	dtd = xmlGetIntSubset(ctxt->myDoc);
	if (dtd == NULL)
	    ctxt->myDoc->intSubset =
		xmlCreateIntSubset(ctxt->myDoc, BAD_CAST "html",
		    BAD_CAST "-//W3C//DTD HTML 4.0 Transitional//EN",
		    BAD_CAST "http://www.w3.org/TR/REC-html40/loose.dtd");
    }
#ifdef DEBUG_PUSH
    xmlGenericError(xmlGenericErrorContext, "HPP: done %d\n", ret);
#endif
    return(ret);
}