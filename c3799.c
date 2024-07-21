xmlParseElementDecl(xmlParserCtxtPtr ctxt) {
    const xmlChar *name;
    int ret = -1;
    xmlElementContentPtr content  = NULL;

    /* GROW; done in the caller */
    if (CMP9(CUR_PTR, '<', '!', 'E', 'L', 'E', 'M', 'E', 'N', 'T')) {
	xmlParserInputPtr input = ctxt->input;

	SKIP(9);
	if (!IS_BLANK_CH(CUR)) {
	    xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
		           "Space required after 'ELEMENT'\n");
	}
        SKIP_BLANKS;
        name = xmlParseName(ctxt);
	if (name == NULL) {
	    xmlFatalErrMsg(ctxt, XML_ERR_NAME_REQUIRED,
			   "xmlParseElementDecl: no name for Element\n");
	    return(-1);
	}
	while ((RAW == 0) && (ctxt->inputNr > 1))
	    xmlPopInput(ctxt);
	if (!IS_BLANK_CH(CUR)) {
	    xmlFatalErrMsg(ctxt, XML_ERR_SPACE_REQUIRED,
			   "Space required after the element name\n");
	}
        SKIP_BLANKS;
	if (CMP5(CUR_PTR, 'E', 'M', 'P', 'T', 'Y')) {
	    SKIP(5);
	    /*
	     * Element must always be empty.
	     */
	    ret = XML_ELEMENT_TYPE_EMPTY;
	} else if ((RAW == 'A') && (NXT(1) == 'N') &&
	           (NXT(2) == 'Y')) {
	    SKIP(3);
	    /*
	     * Element is a generic container.
	     */
	    ret = XML_ELEMENT_TYPE_ANY;
	} else if (RAW == '(') {
	    ret = xmlParseElementContentDecl(ctxt, name, &content);
	} else {
	    /*
	     * [ WFC: PEs in Internal Subset ] error handling.
	     */
	    if ((RAW == '%') && (ctxt->external == 0) &&
	        (ctxt->inputNr == 1)) {
		xmlFatalErrMsg(ctxt, XML_ERR_PEREF_IN_INT_SUBSET,
	  "PEReference: forbidden within markup decl in internal subset\n");
	    } else {
		xmlFatalErrMsg(ctxt, XML_ERR_ELEMCONTENT_NOT_STARTED,
		      "xmlParseElementDecl: 'EMPTY', 'ANY' or '(' expected\n");
            }
	    return(-1);
	}

	SKIP_BLANKS;
	/*
	 * Pop-up of finished entities.
	 */
	while ((RAW == 0) && (ctxt->inputNr > 1))
	    xmlPopInput(ctxt);
	SKIP_BLANKS;

	if (RAW != '>') {
	    xmlFatalErr(ctxt, XML_ERR_GT_REQUIRED, NULL);
	    if (content != NULL) {
		xmlFreeDocElementContent(ctxt->myDoc, content);
	    }
	} else {
	    if (input != ctxt->input) {
		xmlFatalErrMsg(ctxt, XML_ERR_ENTITY_BOUNDARY,
    "Element declaration doesn't start and stop in the same entity\n");
	    }

	    NEXT;
	    if ((ctxt->sax != NULL) && (!ctxt->disableSAX) &&
		(ctxt->sax->elementDecl != NULL)) {
		if (content != NULL)
		    content->parent = NULL;
	        ctxt->sax->elementDecl(ctxt->userData, name, ret,
		                       content);
		if ((content != NULL) && (content->parent == NULL)) {
		    /*
		     * this is a trick: if xmlAddElementDecl is called,
		     * instead of copying the full tree it is plugged directly
		     * if called from the parser. Avoid duplicating the
		     * interfaces or change the API/ABI
		     */
		    xmlFreeDocElementContent(ctxt->myDoc, content);
		}
	    } else if (content != NULL) {
		xmlFreeDocElementContent(ctxt->myDoc, content);
	    }
	}
    }
    return(ret);
}