xmlBufResize(xmlBufPtr buf, size_t size)
{
    unsigned int newSize;
    xmlChar* rebuf = NULL;
    size_t start_buf;

    if ((buf == NULL) || (buf->error))
        return(0);
    CHECK_COMPAT(buf)

    if (buf->alloc == XML_BUFFER_ALLOC_IMMUTABLE) return(0);
    if (buf->alloc == XML_BUFFER_ALLOC_BOUNDED) {
        /*
	 * Used to provide parsing limits
	 */
        if (size >= XML_MAX_TEXT_LENGTH) {
	    xmlBufMemoryError(buf, "buffer error: text too long\n");
	    return(0);
	}
    }

    /* Don't resize if we don't have to */
    if (size < buf->size)
        return 1;

    /* figure out new size */
    switch (buf->alloc){
	case XML_BUFFER_ALLOC_IO:
	case XML_BUFFER_ALLOC_DOUBLEIT:
	    /*take care of empty case*/
	    newSize = (buf->size ? buf->size*2 : size + 10);
	    while (size > newSize) {
	        if (newSize > UINT_MAX / 2) {
	            xmlBufMemoryError(buf, "growing buffer");
	            return 0;
	        }
	        newSize *= 2;
	    }
	    break;
	case XML_BUFFER_ALLOC_EXACT:
	    newSize = size+10;
	    break;
        case XML_BUFFER_ALLOC_HYBRID:
            if (buf->use < BASE_BUFFER_SIZE)
                newSize = size;
            else {
                newSize = buf->size * 2;
                while (size > newSize) {
                    if (newSize > UINT_MAX / 2) {
                        xmlBufMemoryError(buf, "growing buffer");
                        return 0;
                    }
                    newSize *= 2;
                }
            }
            break;

	default:
	    newSize = size+10;
	    break;
    }

    if ((buf->alloc == XML_BUFFER_ALLOC_IO) && (buf->contentIO != NULL)) {
        start_buf = buf->content - buf->contentIO;

        if (start_buf > newSize) {
	    /* move data back to start */
	    memmove(buf->contentIO, buf->content, buf->use);
	    buf->content = buf->contentIO;
	    buf->content[buf->use] = 0;
	    buf->size += start_buf;
	} else {
	    rebuf = (xmlChar *) xmlRealloc(buf->contentIO, start_buf + newSize);
	    if (rebuf == NULL) {
		xmlBufMemoryError(buf, "growing buffer");
		return 0;
	    }
	    buf->contentIO = rebuf;
	    buf->content = rebuf + start_buf;
	}
    } else {
	if (buf->content == NULL) {
	    rebuf = (xmlChar *) xmlMallocAtomic(newSize);
	} else if (buf->size - buf->use < 100) {
	    rebuf = (xmlChar *) xmlRealloc(buf->content, newSize);
        } else {
	    /*
	     * if we are reallocating a buffer far from being full, it's
	     * better to make a new allocation and copy only the used range
	     * and free the old one.
	     */
	    rebuf = (xmlChar *) xmlMallocAtomic(newSize);
	    if (rebuf != NULL) {
		memcpy(rebuf, buf->content, buf->use);
		xmlFree(buf->content);
		rebuf[buf->use] = 0;
	    }
	}
	if (rebuf == NULL) {
	    xmlBufMemoryError(buf, "growing buffer");
	    return 0;
	}
	buf->content = rebuf;
    }
    buf->size = newSize;
    UPDATE_COMPAT(buf)

    return 1;
}