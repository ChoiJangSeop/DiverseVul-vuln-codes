xmlXPtrNewCollapsedRange(xmlNodePtr start) {
    xmlXPathObjectPtr ret;

    if (start == NULL)
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
    ret->user2 = NULL;
    ret->index2 = -1;
    return(ret);
}