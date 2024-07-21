xmlBufAdd(xmlBufPtr buf, const xmlChar *str, int len) {
    unsigned int needSize;

    if ((str == NULL) || (buf == NULL) || (buf->error))
	return -1;
    CHECK_COMPAT(buf)

    if (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return -1;
    if (len < -1) {
#ifdef DEBUG_BUFFER
        xmlGenericError(xmlGenericErrorContext,
		"xmlBufAdd: len < 0\n");
#endif
	return -1;
    }
    if (len == 0) return 0;

    if (len < 0)
        len = xmlStrlen(str);

    if (len < 0) return -1;
    if (len == 0) return 0;

    needSize = buf->use + len + 2;
    if (needSize > buf->size){
        if (!xmlBufResize(buf, needSize)){
	    xmlBufMemoryError(buf, "growing buffer");
            return XML_ERR_NO_MEMORY;
        }
    }

    memmove(&buf->content[buf->use], str, len*sizeof(xmlChar));
    buf->use += len;
    buf->content[buf->use] = 0;
    UPDATE_COMPAT(buf)
    return 0;
}