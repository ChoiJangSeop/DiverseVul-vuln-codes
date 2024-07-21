HttpStateData::continueAfterParsingHeader()
{
    if (flags.handling1xx) {
        debugs(11, 5, HERE << "wait for 1xx handling");
        Must(!flags.headers_parsed);
        return false;
    }

    if (!flags.headers_parsed && !eof) {
        debugs(11, 9, "needs more at " << inBuf.length());
        flags.do_next_read = true;
        /** \retval false If we have not finished parsing the headers and may get more data.
         *                Schedules more reads to retrieve the missing data.
         */
        maybeReadVirginBody(); // schedules all kinds of reads; TODO: rename
        return false;
    }

    /** If we are done with parsing, check for errors */

    err_type error = ERR_NONE;

    if (flags.headers_parsed) { // parsed headers, possibly with errors
        // check for header parsing errors
        if (HttpReply *vrep = virginReply()) {
            const Http::StatusCode s = vrep->sline.status();
            const AnyP::ProtocolVersion &v = vrep->sline.version;
            if (s == Http::scInvalidHeader && v != Http::ProtocolVersion(0,9)) {
                debugs(11, DBG_IMPORTANT, "WARNING: HTTP: Invalid Response: Bad header encountered from " << entry->url() << " AKA " << request->url);
                error = ERR_INVALID_RESP;
            } else if (s == Http::scHeaderTooLarge) {
                fwd->dontRetry(true);
                error = ERR_TOO_BIG;
            } else if (vrep->header.conflictingContentLength()) {
                fwd->dontRetry(true);
                error = ERR_INVALID_RESP;
            } else {
                return true; // done parsing, got reply, and no error
            }
        } else {
            // parsed headers but got no reply
            debugs(11, DBG_IMPORTANT, "WARNING: HTTP: Invalid Response: No reply at all for " << entry->url() << " AKA " << request->url);
            error = ERR_INVALID_RESP;
        }
    } else {
        assert(eof);
        if (inBuf.length()) {
            error = ERR_INVALID_RESP;
            debugs(11, DBG_IMPORTANT, "WARNING: HTTP: Invalid Response: Headers did not parse at all for " << entry->url() << " AKA " << request->url);
        } else {
            error = ERR_ZERO_SIZE_OBJECT;
            debugs(11, (request->flags.accelerated?DBG_IMPORTANT:2), "WARNING: HTTP: Invalid Response: No object data received for " << entry->url() << " AKA " << request->url);
        }
    }

    assert(error != ERR_NONE);
    entry->reset();
    fwd->fail(new ErrorState(error, Http::scBadGateway, fwd->request));
    flags.do_next_read = false;
    closeServer();
    mustStop("HttpStateData::continueAfterParsingHeader");
    return false; // quit on error
}