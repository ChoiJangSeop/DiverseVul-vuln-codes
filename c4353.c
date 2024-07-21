xmlXPtrNewRangeNodeObject(xmlNodePtr start, xmlXPathObjectPtr end) {
    xmlXPathObjectPtr ret;

    if (start == NULL)
	return(NULL);
    if (end == NULL)
	return(NULL);
    switch (end->type) {
	case XPATH_POINT:
	case XPATH_RANGE:
	    break;
	case XPATH_NODESET:
	    /*
	     * Empty set ...
	     */
	    if (end->nodesetval->nodeNr <= 0)
		return(NULL);
	    break;
	default:
	    /* TODO */
	    return(NULL);
    }

    ret = (xmlXPathObjectPtr) xmlMalloc(sizeof(xmlXPathObject));
    if (ret == NULL) {
        xmlXPtrErrMemory("allocating range");
	return(NULL);
    }
    memset(ret, 0 , (size_t) sizeof(xmlXPathObject));
    ret->type = XPATH_RANGE;
    ret->user = start;
    ret->index = -1;
    switch (end->type) {
	case XPATH_POINT:
	    ret->user2 = end->user;
	    ret->index2 = end->index;
	    break;
	case XPATH_RANGE:
	    ret->user2 = end->user2;
	    ret->index2 = end->index2;
	    break;
	case XPATH_NODESET: {
	    ret->user2 = end->nodesetval->nodeTab[end->nodesetval->nodeNr - 1];
	    ret->index2 = -1;
	    break;
	}
	default:
	    STRANGE
	    return(NULL);
    }
    xmlXPtrRangeCheckOrder(ret);
    return(ret);
}