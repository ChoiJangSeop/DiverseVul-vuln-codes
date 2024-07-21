xmlXPtrRangeToFunction(xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr range;
    const xmlChar *cur;
    xmlXPathObjectPtr res, obj;
    xmlXPathObjectPtr tmp;
    xmlLocationSetPtr newset = NULL;
    xmlNodeSetPtr oldset;
    int i;

    if (ctxt == NULL) return;
    CHECK_ARITY(1);
    /*
     * Save the expression pointer since we will have to evaluate
     * it multiple times. Initialize the new set.
     */
    CHECK_TYPE(XPATH_NODESET);
    obj = valuePop(ctxt);
    oldset = obj->nodesetval;
    ctxt->context->node = NULL;

    cur = ctxt->cur;
    newset = xmlXPtrLocationSetCreate(NULL);

    for (i = 0; i < oldset->nodeNr; i++) {
	ctxt->cur = cur;

	/*
	 * Run the evaluation with a node list made of a single item
	 * in the nodeset.
	 */
	ctxt->context->node = oldset->nodeTab[i];
	tmp = xmlXPathNewNodeSet(ctxt->context->node);
	valuePush(ctxt, tmp);

	xmlXPathEvalExpr(ctxt);
	CHECK_ERROR;

	/*
	 * The result of the evaluation need to be tested to
	 * decided whether the filter succeeded or not
	 */
	res = valuePop(ctxt);
	range = xmlXPtrNewRangeNodeObject(oldset->nodeTab[i], res);
	if (range != NULL) {
	    xmlXPtrLocationSetAdd(newset, range);
	}

	/*
	 * Cleanup
	 */
	if (res != NULL)
	    xmlXPathFreeObject(res);
	if (ctxt->value == tmp) {
	    res = valuePop(ctxt);
	    xmlXPathFreeObject(res);
	}

	ctxt->context->node = NULL;
    }

    /*
     * The result is used as the new evaluation set.
     */
    xmlXPathFreeObject(obj);
    ctxt->context->node = NULL;
    ctxt->context->contextSize = -1;
    ctxt->context->proximityPosition = -1;
    valuePush(ctxt, xmlXPtrWrapLocationSet(newset));
}