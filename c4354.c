xmlXPtrNewRange(xmlNodePtr start, int startindex,
	        xmlNodePtr end, int endindex) {
    xmlXPathObjectPtr ret;

    if (start == NULL)
	return(NULL);
    if (end == NULL)
	return(NULL);
    if (startindex < 0)
	return(NULL);
    if (endindex < 0)
	return(NULL);

    ret = (xmlXPathObjectPtr) xmlMalloc(sizeof(xmlXPathObject));
    if (ret == NULL) {
        xmlXPtrErrMemory("allocating range");
	return(NULL);
    }
    memset(ret, 0 , (size_t) sizeof(xmlXPathObject));
    ret->type = XPATH_RANGE;
    ret->user = start;
    ret->index = startindex;
    ret->user2 = end;
    ret->index2 = endindex;
    xmlXPtrRangeCheckOrder(ret);
    return(ret);
}