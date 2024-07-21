xmlBufferCreateStatic(void *mem, size_t size) {
    xmlBufferPtr ret;

    if ((mem == NULL) || (size == 0))
        return(NULL);

    ret = (xmlBufferPtr) xmlMalloc(sizeof(xmlBuffer));
    if (ret == NULL) {
	xmlTreeErrMemory("creating buffer");
        return(NULL);
    }
    ret->use = size;
    ret->size = size;
    ret->alloc = XML_BUFFER_ALLOC_IMMUTABLE;
    ret->content = (xmlChar *) mem;
    return(ret);
}