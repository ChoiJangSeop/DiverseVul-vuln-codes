xmlXPtrNewRangePointNode(xmlXPathObjectPtr start, xmlNodePtr end) {
    xmlXPathObjectPtr ret;

    if (start == NULL)
	return(NULL);
    if (end == NULL)
	return(NULL);
    if (start->type != XPATH_POINT)
	return(NULL);

    ret = (xmlXPathObjectPtr) xmlMalloc(sizeof(xmlXPathObject));
    if (ret == NULL) {
        xmlXPtrErrMemory("allocating range");
	return(NULL);
    }
    memset(ret, 0 , (size_t) sizeof(xmlXPathObject));
    ret->type = XPATH_RANGE;
    ret->user = start->user;
    ret->index = start->index;
    ret->user2 = end;
    ret->index2 = -1;
    xmlXPtrRangeCheckOrder(ret);
    return(ret);
}