htmlParseSystemLiteral(htmlParserCtxtPtr ctxt) {
    const xmlChar *q;
    xmlChar *ret = NULL;

    if (CUR == '"') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR_CH(CUR)) && (CUR != '"'))
	    NEXT;
	if (!IS_CHAR_CH(CUR)) {
	    htmlParseErr(ctxt, XML_ERR_LITERAL_NOT_FINISHED,
			 "Unfinished SystemLiteral\n", NULL, NULL);
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else if (CUR == '\'') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR_CH(CUR)) && (CUR != '\''))
	    NEXT;
	if (!IS_CHAR_CH(CUR)) {
	    htmlParseErr(ctxt, XML_ERR_LITERAL_NOT_FINISHED,
			 "Unfinished SystemLiteral\n", NULL, NULL);
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else {
	htmlParseErr(ctxt, XML_ERR_LITERAL_NOT_STARTED,
	             " or ' expected\n", NULL, NULL);
    }

    return(ret);
}