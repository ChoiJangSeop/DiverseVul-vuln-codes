clientInterpretRequestHeaders(ClientHttpRequest * http)
{
    HttpRequest *request = http->request;
    HttpHeader *req_hdr = &request->header;
    bool no_cache = false;

    request->imslen = -1;
    request->ims = req_hdr->getTime(Http::HdrType::IF_MODIFIED_SINCE);

    if (request->ims > 0)
        request->flags.ims = true;

    if (!request->flags.ignoreCc) {
        if (request->cache_control) {
            if (request->cache_control->hasNoCache())
                no_cache=true;

            // RFC 2616: treat Pragma:no-cache as if it was Cache-Control:no-cache when Cache-Control is missing
        } else if (req_hdr->has(Http::HdrType::PRAGMA))
            no_cache = req_hdr->hasListMember(Http::HdrType::PRAGMA,"no-cache",',');
    }

    if (request->method == Http::METHOD_OTHER) {
        no_cache=true;
    }

    if (no_cache) {
#if USE_HTTP_VIOLATIONS

        if (Config.onoff.reload_into_ims)
            request->flags.nocacheHack = true;
        else if (refresh_nocache_hack)
            request->flags.nocacheHack = true;
        else
#endif

            request->flags.noCache = true;
    }

    /* ignore range header in non-GETs or non-HEADs */
    if (request->method == Http::METHOD_GET || request->method == Http::METHOD_HEAD) {
        // XXX: initialize if we got here without HttpRequest::parseHeader()
        if (!request->range)
            request->range = req_hdr->getRange();

        if (request->range) {
            request->flags.isRanged = true;
            clientStreamNode *node = (clientStreamNode *)http->client_stream.tail->data;
            /* XXX: This is suboptimal. We should give the stream the range set,
             * and thereby let the top of the stream set the offset when the
             * size becomes known. As it is, we will end up requesting from 0
             * for evey -X range specification.
             * RBC - this may be somewhat wrong. We should probably set the range
             * iter up at this point.
             */
            node->readBuffer.offset = request->range->lowestOffset(0);
            http->range_iter.pos = request->range->begin();
            http->range_iter.end = request->range->end();
            http->range_iter.valid = true;
        }
    }

    /* Only HEAD and GET requests permit a Range or Request-Range header.
     * If these headers appear on any other type of request, delete them now.
     */
    else {
        req_hdr->delById(Http::HdrType::RANGE);
        req_hdr->delById(Http::HdrType::REQUEST_RANGE);
        request->ignoreRange("neither HEAD nor GET");
    }

    if (req_hdr->has(Http::HdrType::AUTHORIZATION))
        request->flags.auth = true;

    clientCheckPinning(http);

    if (!request->url.userInfo().isEmpty())
        request->flags.auth = true;

    if (req_hdr->has(Http::HdrType::VIA)) {
        String s = req_hdr->getList(Http::HdrType::VIA);
        /*
         * ThisCache cannot be a member of Via header, "1.1 ThisCache" can.
         * Note ThisCache2 has a space prepended to the hostname so we don't
         * accidentally match super-domains.
         */

        if (strListIsSubstr(&s, ThisCache2, ',')) {
            request->flags.loopDetected = true;
        }

#if USE_FORW_VIA_DB
        fvdbCountVia(s.termedBuf());

#endif

        s.clean();
    }

    // headers only relevant to reverse-proxy
    if (request->flags.accelerated) {
        // check for a cdn-info member with a cdn-id matching surrogate_id
        // XXX: HttpHeader::hasListMember() does not handle OWS around ";" yet
        if (req_hdr->hasListMember(Http::HdrType::CDN_LOOP, Config.Accel.surrogate_id, ','))
            request->flags.loopDetected = true;
    }

    if (request->flags.loopDetected) {
        debugObj(33, DBG_IMPORTANT, "WARNING: Forwarding loop detected for:\n",
                 request, (ObjPackMethod) & httpRequestPack);
    }

#if USE_FORW_VIA_DB

    if (req_hdr->has(Http::HdrType::X_FORWARDED_FOR)) {
        String s = req_hdr->getList(Http::HdrType::X_FORWARDED_FOR);
        fvdbCountForw(s.termedBuf());
        s.clean();
    }

#endif

    request->flags.cachable = http->request->maybeCacheable();

    if (clientHierarchical(http))
        request->flags.hierarchical = true;

    debugs(85, 5, "clientInterpretRequestHeaders: REQ_NOCACHE = " <<
           (request->flags.noCache ? "SET" : "NOT SET"));
    debugs(85, 5, "clientInterpretRequestHeaders: REQ_CACHABLE = " <<
           (request->flags.cachable ? "SET" : "NOT SET"));
    debugs(85, 5, "clientInterpretRequestHeaders: REQ_HIERARCHICAL = " <<
           (request->flags.hierarchical ? "SET" : "NOT SET"));

}