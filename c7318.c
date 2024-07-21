ssize_t _libssh2_channel_read(LIBSSH2_CHANNEL *channel, int stream_id,
                              char *buf, size_t buflen)
{
    LIBSSH2_SESSION *session = channel->session;
    int rc;
    int bytes_read = 0;
    int bytes_want;
    int unlink_packet;
    LIBSSH2_PACKET *read_packet;
    LIBSSH2_PACKET *read_next;

    _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                   "channel_read() wants %d bytes from channel %lu/%lu "
                   "stream #%d",
                   (int) buflen, channel->local.id, channel->remote.id,
                   stream_id);

    /* expand the receiving window first if it has become too narrow */
    if((channel->read_state == libssh2_NB_state_jump1) ||
       (channel->remote.window_size < channel->remote.window_size_initial / 4 * 3 + buflen) ) {

        uint32_t adjustment = channel->remote.window_size_initial + buflen - channel->remote.window_size;
        if(adjustment < LIBSSH2_CHANNEL_MINADJUST)
            adjustment = LIBSSH2_CHANNEL_MINADJUST;

        /* the actual window adjusting may not finish so we need to deal with
           this special state here */
        channel->read_state = libssh2_NB_state_jump1;
        rc = _libssh2_channel_receive_window_adjust(channel, adjustment,
                                                    0, NULL);
        if(rc)
            return rc;

        channel->read_state = libssh2_NB_state_idle;
    }

    /* Process all pending incoming packets. Tests prove that this way
       produces faster transfers. */
    do {
        rc = _libssh2_transport_read(session);
    } while(rc > 0);

    if((rc < 0) && (rc != LIBSSH2_ERROR_EAGAIN))
        return _libssh2_error(session, rc, "transport read");

    read_packet = _libssh2_list_first(&session->packets);
    while(read_packet && (bytes_read < (int) buflen)) {
        /* previously this loop condition also checked for
           !channel->remote.close but we cannot let it do this:

           We may have a series of packets to read that are still pending even
           if a close has been received. Acknowledging the close too early
           makes us flush buffers prematurely and loose data.
        */

        LIBSSH2_PACKET *readpkt = read_packet;

        /* In case packet gets destroyed during this iteration */
        read_next = _libssh2_list_next(&readpkt->node);

        channel->read_local_id =
            _libssh2_ntohu32(readpkt->data + 1);

        /*
         * Either we asked for a specific extended data stream
         * (and data was available),
         * or the standard stream (and data was available),
         * or the standard stream with extended_data_merge
         * enabled and data was available
         */
        if((stream_id
             && (readpkt->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
             && (channel->local.id == channel->read_local_id)
             && (stream_id == (int) _libssh2_ntohu32(readpkt->data + 5)))
            || (!stream_id && (readpkt->data[0] == SSH_MSG_CHANNEL_DATA)
                && (channel->local.id == channel->read_local_id))
            || (!stream_id
                && (readpkt->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
                && (channel->local.id == channel->read_local_id)
                && (channel->remote.extended_data_ignore_mode ==
                    LIBSSH2_CHANNEL_EXTENDED_DATA_MERGE))) {

            /* figure out much more data we want to read */
            bytes_want = buflen - bytes_read;
            unlink_packet = FALSE;

            if(bytes_want >= (int) (readpkt->data_len - readpkt->data_head)) {
                /* we want more than this node keeps, so adjust the number and
                   delete this node after the copy */
                bytes_want = readpkt->data_len - readpkt->data_head;
                unlink_packet = TRUE;
            }

            _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                           "channel_read() got %d of data from %lu/%lu/%d%s",
                           bytes_want, channel->local.id,
                           channel->remote.id, stream_id,
                           unlink_packet?" [ul]":"");

            /* copy data from this struct to the target buffer */
            memcpy(&buf[bytes_read],
                   &readpkt->data[readpkt->data_head], bytes_want);

            /* advance pointer and counter */
            readpkt->data_head += bytes_want;
            bytes_read += bytes_want;

            /* if drained, remove from list */
            if(unlink_packet) {
                /* detach readpkt from session->packets list */
                _libssh2_list_remove(&readpkt->node);

                LIBSSH2_FREE(session, readpkt->data);
                LIBSSH2_FREE(session, readpkt);
            }
        }

        /* check the next struct in the chain */
        read_packet = read_next;
    }

    if(!bytes_read) {
        /* If the channel is already at EOF or even closed, we need to signal
           that back. We may have gotten that info while draining the incoming
           transport layer until EAGAIN so we must not be fooled by that
           return code. */
        if(channel->remote.eof || channel->remote.close)
            return 0;
        else if(rc != LIBSSH2_ERROR_EAGAIN)
            return 0;

        /* if the transport layer said EAGAIN then we say so as well */
        return _libssh2_error(session, rc, "would block");
    }

    channel->read_avail -= bytes_read;
    channel->remote.window_size -= bytes_read;

    return bytes_read;
}