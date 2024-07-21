Http::Stream::getNextRangeOffset() const
{
    debugs (33, 5, "range: " << http->request->range <<
            "; http offset " << http->out.offset <<
            "; reply " << reply);

    // XXX: This method is called from many places, including pullData() which
    // may be called before prepareReply() [on some Squid-generated errors].
    // Hence, we may not even know yet whether we should honor/do ranges.

    if (http->request->range) {
        /* offset in range specs does not count the prefix of an http msg */
        /* check: reply was parsed and range iterator was initialized */
        assert(http->range_iter.valid);
        /* filter out data according to range specs */
        assert(canPackMoreRanges());
        {
            assert(http->range_iter.currentSpec());
            /* offset of still missing data */
            int64_t start = http->range_iter.currentSpec()->offset +
                            http->range_iter.currentSpec()->length -
                            http->range_iter.debt();
            debugs(33, 3, "clientPackMoreRanges: in:  offset: " << http->out.offset);
            debugs(33, 3, "clientPackMoreRanges: out:"
                   " start: " << start <<
                   " spec[" << http->range_iter.pos - http->request->range->begin() << "]:" <<
                   " [" << http->range_iter.currentSpec()->offset <<
                   ", " << http->range_iter.currentSpec()->offset +
                   http->range_iter.currentSpec()->length << "),"
                   " len: " << http->range_iter.currentSpec()->length <<
                   " debt: " << http->range_iter.debt());
            if (http->range_iter.currentSpec()->length != -1)
                assert(http->out.offset <= start);  /* we did not miss it */

            return start;
        }

    } else if (reply && reply->contentRange()) {
        /* request does not have ranges, but reply does */
        /* TODO: should use range_iter_pos on reply, as soon as reply->content_range
         *        becomes HttpHdrRange rather than HttpHdrRangeSpec.
         */
        return http->out.offset + reply->contentRange()->spec.offset;
    }

    return http->out.offset;
}