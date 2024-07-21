xmlBufAddHead(xmlBufPtr buf, const xmlChar *str, int len) {
    unsigned int needSize;

    if ((buf == NULL) || (buf->error))
        return(-1);
    CHECK_COMPAT(buf)
    if (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return -1;
    if (str == NULL) {
#ifdef DEBUG_BUFFER
        xmlGenericError(xmlGenericErrorContext,
		"xmlBufAddHead: str == NULL\n");
#endif
	return -1;
    }
    if (len < -1) {
#ifdef DEBUG_BUFFER
        xmlGenericError(xmlGenericErrorContext,
		"xmlBufAddHead: len < 0\n");
#endif
	return -1;
    }
    if (len == 0) return 0;

    if (len < 0)
        len = xmlStrlen(str);

    if (len <= 0) return -1;

    if ((buf->alloc == XML_BUFFER_ALLOC_IO) && (buf->contentIO != NULL)) {
        size_t start_buf = buf->content - buf->contentIO;

	if (start_buf > (unsigned int) len) {
	    /*
	     * We can add it in the space previously shrinked
	     */
	    buf->content -= len;
            memmove(&buf->content[0], str, len);
	    buf->use += len;
	    buf->size += len;
	    UPDATE_COMPAT(buf)
	    return(0);
	}
    }
    needSize = buf->use + len + 2;
    if (needSize > buf->size){
        if (!xmlBufResize(buf, needSize)){
	    xmlBufMemoryError(buf, "growing buffer");
            return XML_ERR_NO_MEMORY;
        }
    }

    memmove(&buf->content[len], &buf->content[0], buf->use);
    memmove(&buf->content[0], str, len);
    buf->use += len;
    buf->content[buf->use] = 0;
    UPDATE_COMPAT(buf)
    return 0;
}