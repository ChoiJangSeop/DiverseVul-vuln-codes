ClientHttpRequest::mRangeCLen()
{
    int64_t clen = 0;
    MemBuf mb;

    assert(memObject());

    mb.init();
    HttpHdrRange::iterator pos = request->range->begin();

    while (pos != request->range->end()) {
        /* account for headers for this range */
        mb.reset();
        clientPackRangeHdr(memObject()->getReply(),
                           *pos, range_iter.boundary, &mb);
        clen += mb.size;

        /* account for range content */
        clen += (*pos)->length;

        debugs(33, 6, "clientMRangeCLen: (clen += " << mb.size << " + " << (*pos)->length << ") == " << clen);
        ++pos;
    }

    /* account for the terminating boundary */
    mb.reset();

    clientPackTermBound(range_iter.boundary, &mb);

    clen += mb.size;

    mb.clean();

    return clen;
}