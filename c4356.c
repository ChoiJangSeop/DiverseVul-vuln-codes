xmlXPtrNewRangeNodes(xmlNodePtr start, xmlNodePtr end) {
    xmlXPathObjectPtr ret;

    if (start == NULL)
	return(NULL);
    if (end == NULL)
	return(NULL);

    ret = (xmlXPathObjectPtr) xmlMalloc(sizeof(xmlXPathObject));
    if (ret == NULL) {
        xmlXPtrErrMemory("allocating range");
	return(NULL);
    }
    memset(ret, 0 , (size_t) sizeof(xmlXPathObject));
    ret->type = XPATH_RANGE;
    ret->user = start;
    ret->index = -1;
    ret->user2 = end;
    ret->index2 = -1;
    xmlXPtrRangeCheckOrder(ret);
    return(ret);
}