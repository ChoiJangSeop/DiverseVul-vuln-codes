 */
static xmlXPathParserContextPtr
xmlXPathCompParserContext(xmlXPathCompExprPtr comp, xmlXPathContextPtr ctxt) {
    xmlXPathParserContextPtr ret;

    ret = (xmlXPathParserContextPtr) xmlMalloc(sizeof(xmlXPathParserContext));
    if (ret == NULL) {
        xmlXPathErrMemory(ctxt, "creating evaluation context\n");
	return(NULL);
    }
    memset(ret, 0 , (size_t) sizeof(xmlXPathParserContext));

    /* Allocate the value stack */
    ret->valueTab = (xmlXPathObjectPtr *)
                     xmlMalloc(10 * sizeof(xmlXPathObjectPtr));
    if (ret->valueTab == NULL) {
	xmlFree(ret);
	xmlXPathErrMemory(ctxt, "creating evaluation context\n");
	return(NULL);
    }
    ret->valueNr = 0;
    ret->valueMax = 10;
    ret->value = NULL;

    ret->context = ctxt;
    ret->comp = comp;
