Http::Stream::socketState()
{
    switch (clientStreamStatus(getTail(), http)) {

    case STREAM_NONE:
        /* check for range support ending */
        if (http->request->range) {
            /* check: reply was parsed and range iterator was initialized */
            assert(http->range_iter.valid);
            /* filter out data according to range specs */

            if (!canPackMoreRanges()) {
                debugs(33, 5, "Range request at end of returnable " <<
                       "range sequence on " << clientConnection);
                // we got everything we wanted from the store
                return STREAM_COMPLETE;
            }
        } else if (reply && reply->contentRange()) {
            /* reply has content-range, but Squid is not managing ranges */
            const int64_t &bytesSent = http->out.offset;
            const int64_t &bytesExpected = reply->contentRange()->spec.length;

            debugs(33, 7, "body bytes sent vs. expected: " <<
                   bytesSent << " ? " << bytesExpected << " (+" <<
                   reply->contentRange()->spec.offset << ")");

            // did we get at least what we expected, based on range specs?

            if (bytesSent == bytesExpected) // got everything
                return STREAM_COMPLETE;

            if (bytesSent > bytesExpected) // Error: Sent more than expected
                return STREAM_UNPLANNED_COMPLETE;
        }

        return STREAM_NONE;

    case STREAM_COMPLETE:
        return STREAM_COMPLETE;

    case STREAM_UNPLANNED_COMPLETE:
        return STREAM_UNPLANNED_COMPLETE;

    case STREAM_FAILED:
        return STREAM_FAILED;
    }

    fatal ("unreachable code\n");
    return STREAM_NONE;
}