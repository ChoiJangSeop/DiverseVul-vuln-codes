Http::Stream::buildRangeHeader(HttpReply *rep)
{
    HttpHeader *hdr = rep ? &rep->header : nullptr;
    const char *range_err = nullptr;
    HttpRequest *request = http->request;
    assert(request->range);
    /* check if we still want to do ranges */
    int64_t roffLimit = request->getRangeOffsetLimit();
    auto contentRange = rep ? rep->contentRange() : nullptr;

    if (!rep)
        range_err = "no [parse-able] reply";
    else if ((rep->sline.status() != Http::scOkay) && (rep->sline.status() != Http::scPartialContent))
        range_err = "wrong status code";
    else if (rep->sline.status() == Http::scPartialContent)
        range_err = "too complex response"; // probably contains what the client needs
    else if (rep->sline.status() != Http::scOkay)
        range_err = "wrong status code";
    else if (hdr->has(Http::HdrType::CONTENT_RANGE)) {
        Must(!contentRange); // this is a 200, not 206 response
        range_err = "meaningless response"; // the status code or the header is wrong
    }
    else if (rep->content_length < 0)
        range_err = "unknown length";
    else if (rep->content_length != http->memObject()->getReply()->content_length)
        range_err = "INCONSISTENT length";  /* a bug? */

    /* hits only - upstream CachePeer determines correct behaviour on misses,
     * and client_side_reply determines hits candidates
     */
    else if (http->logType.isTcpHit() &&
             http->request->header.has(Http::HdrType::IF_RANGE) &&
             !clientIfRangeMatch(http, rep))
        range_err = "If-Range match failed";

    else if (!http->request->range->canonize(rep))
        range_err = "canonization failed";
    else if (http->request->range->isComplex())
        range_err = "too complex range header";
    else if (!http->logType.isTcpHit() && http->request->range->offsetLimitExceeded(roffLimit))
        range_err = "range outside range_offset_limit";

    /* get rid of our range specs on error */
    if (range_err) {
        /* XXX We do this here because we need canonisation etc. However, this current
         * code will lead to incorrect store offset requests - the store will have the
         * offset data, but we won't be requesting it.
         * So, we can either re-request, or generate an error
         */
        http->request->ignoreRange(range_err);
    } else {
        /* XXX: TODO: Review, this unconditional set may be wrong. */
        rep->sline.set(rep->sline.version, Http::scPartialContent);
        // web server responded with a valid, but unexpected range.
        // will (try-to) forward as-is.
        //TODO: we should cope with multirange request/responses
        // TODO: review, since rep->content_range is always nil here.
        bool replyMatchRequest = contentRange != nullptr ?
                                 request->range->contains(contentRange->spec) :
                                 true;
        const int spec_count = http->request->range->specs.size();
        int64_t actual_clen = -1;

        debugs(33, 3, "range spec count: " << spec_count <<
               " virgin clen: " << rep->content_length);
        assert(spec_count > 0);
        /* append appropriate header(s) */
        if (spec_count == 1) {
            if (!replyMatchRequest) {
                hdr->putContRange(contentRange);
                actual_clen = rep->content_length;
                //http->range_iter.pos = rep->content_range->spec.begin();
                (*http->range_iter.pos)->offset = contentRange->spec.offset;
                (*http->range_iter.pos)->length = contentRange->spec.length;

            } else {
                HttpHdrRange::iterator pos = http->request->range->begin();
                assert(*pos);
                /* append Content-Range */

                if (!contentRange) {
                    /* No content range, so this was a full object we are
                     * sending parts of.
                     */
                    httpHeaderAddContRange(hdr, **pos, rep->content_length);
                }

                /* set new Content-Length to the actual number of bytes
                 * transmitted in the message-body */
                actual_clen = (*pos)->length;
            }
        } else {
            /* multipart! */
            /* generate boundary string */
            http->range_iter.boundary = http->rangeBoundaryStr();
            /* delete old Content-Type, add ours */
            hdr->delById(Http::HdrType::CONTENT_TYPE);
            httpHeaderPutStrf(hdr, Http::HdrType::CONTENT_TYPE,
                              "multipart/byteranges; boundary=\"" SQUIDSTRINGPH "\"",
                              SQUIDSTRINGPRINT(http->range_iter.boundary));
            /* Content-Length is not required in multipart responses
             * but it is always nice to have one */
            actual_clen = http->mRangeCLen();

            /* http->out needs to start where we want data at */
            http->out.offset = http->range_iter.currentSpec()->offset;
        }

        /* replace Content-Length header */
        assert(actual_clen >= 0);
        hdr->delById(Http::HdrType::CONTENT_LENGTH);
        hdr->putInt64(Http::HdrType::CONTENT_LENGTH, actual_clen);
        debugs(33, 3, "actual content length: " << actual_clen);

        /* And start the range iter off */
        http->range_iter.updateSpec();
    }
}