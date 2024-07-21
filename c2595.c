xmlBufGrowInternal(xmlBufPtr buf, size_t len) {
    size_t size;
    xmlChar *newbuf;

    if ((buf == NULL) || (buf->error != 0)) return(0);
    CHECK_COMPAT(buf)

    if (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return(0);
    if (buf->use + len < buf->size)
        return(buf->size - buf->use);

    /*
     * Windows has a BIG problem on realloc timing, so we try to double
     * the buffer size (if that's enough) (bug 146697)
     * Apparently BSD too, and it's probably best for linux too
     * On an embedded system this may be something to change
     */
#if 1
    if (buf->size > (size_t) len)
        size = buf->size * 2;
    else
        size = buf->use + len + 100;
#else
    size = buf->use + len + 100;
#endif

    if ((buf->alloc == XML_BUFFER_ALLOC_IO) && (buf->contentIO != NULL)) {
        size_t start_buf = buf->content - buf->contentIO;

	newbuf = (xmlChar *) xmlRealloc(buf->contentIO, start_buf + size);
	if (newbuf == NULL) {
	    xmlBufMemoryError(buf, "growing buffer");
	    return(0);
	}
	buf->contentIO = newbuf;
	buf->content = newbuf + start_buf;
    } else {
	newbuf = (xmlChar *) xmlRealloc(buf->content, size);
	if (newbuf == NULL) {
	    xmlBufMemoryError(buf, "growing buffer");
	    return(0);
	}
	buf->content = newbuf;
    }
    buf->size = size;
    UPDATE_COMPAT(buf)
    return(buf->size - buf->use);
}