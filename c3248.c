xmlParseMarkupDecl(xmlParserCtxtPtr ctxt) {
    GROW;
    if (CUR == '<') {
        if (NXT(1) == '!') {
	    switch (NXT(2)) {
	        case 'E':
		    if (NXT(3) == 'L')
			xmlParseElementDecl(ctxt);
		    else if (NXT(3) == 'N')
			xmlParseEntityDecl(ctxt);
		    break;
	        case 'A':
		    xmlParseAttributeListDecl(ctxt);
		    break;
	        case 'N':
		    xmlParseNotationDecl(ctxt);
		    break;
	        case '-':
		    xmlParseComment(ctxt);
		    break;
		default:
		    /* there is an error but it will be detected later */
		    break;
	    }
	} else if (NXT(1) == '?') {
	    xmlParsePI(ctxt);
	}
    }
    /*
     * This is only for internal subset. On external entities,
     * the replacement is done before parsing stage
     */
    if ((ctxt->external == 0) && (ctxt->inputNr == 1))
	xmlParsePEReference(ctxt);

    /*
     * Conditional sections are allowed from entities included
     * by PE References in the internal subset.
     */
    if ((ctxt->external == 0) && (ctxt->inputNr > 1)) {
        if ((RAW == '<') && (NXT(1) == '!') && (NXT(2) == '[')) {
	    xmlParseConditionalSections(ctxt);
	}
    }

    ctxt->instate = XML_PARSER_DTD;
}