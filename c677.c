 */
static int
xmlXPathRunEval(xmlXPathParserContextPtr ctxt, int toBool)
{
    xmlXPathCompExprPtr comp;

    if ((ctxt == NULL) || (ctxt->comp == NULL))
	return(-1);

    if (ctxt->valueTab == NULL) {
	/* Allocate the value stack */
	ctxt->valueTab = (xmlXPathObjectPtr *)
			 xmlMalloc(10 * sizeof(xmlXPathObjectPtr));
	if (ctxt->valueTab == NULL) {
	    xmlXPathPErrMemory(ctxt, "creating evaluation context\n");
	    xmlFree(ctxt);
	}
	ctxt->valueNr = 0;
	ctxt->valueMax = 10;
	ctxt->value = NULL;
    }
#ifdef XPATH_STREAMING
    if (ctxt->comp->stream) {
	int res;

	if (toBool) {
	    /*
	    * Evaluation to boolean result.
	    */
	    res = xmlXPathRunStreamEval(ctxt->context,
		ctxt->comp->stream, NULL, 1);
	    if (res != -1)
		return(res);
	} else {
	    xmlXPathObjectPtr resObj = NULL;

	    /*
	    * Evaluation to a sequence.
	    */
	    res = xmlXPathRunStreamEval(ctxt->context,
		ctxt->comp->stream, &resObj, 0);

	    if ((res != -1) && (resObj != NULL)) {
		valuePush(ctxt, resObj);
		return(0);
	    }
	    if (resObj != NULL)
		xmlXPathReleaseObject(ctxt->context, resObj);
	}
	/*
	* QUESTION TODO: This falls back to normal XPath evaluation
	* if res == -1. Is this intended?
	*/
    }
#endif
    comp = ctxt->comp;
    if (comp->last < 0) {
	xmlGenericError(xmlGenericErrorContext,
	    "xmlXPathRunEval: last is less than zero\n");
	return(-1);
    }
    if (toBool)
	return(xmlXPathCompOpEvalToBoolean(ctxt,
	    &comp->steps[comp->last], 0));
    else
	xmlXPathCompOpEval(ctxt, &comp->steps[comp->last]);
