xmlBufferCCat(xmlBufferPtr buf, const char *str) {
    const char *cur;

    if (buf == NULL)
        return(-1);
    if (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return -1;
    if (str == NULL) {
#ifdef DEBUG_BUFFER
        xmlGenericError(xmlGenericErrorContext,
		"xmlBufferCCat: str == NULL\n");
#endif
	return -1;
    }
    for (cur = str;*cur != 0;cur++) {
        if (buf->use  + 10 >= buf->size) {
            if (!xmlBufferResize(buf, buf->use+10)){
		xmlTreeErrMemory("growing buffer");
                return XML_ERR_NO_MEMORY;
            }
        }
        buf->content[buf->use++] = *cur;
    }
    buf->content[buf->use] = 0;
    return 0;
}