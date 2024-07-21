_libssh2_channel_flush(LIBSSH2_CHANNEL *channel, int streamid)
{
    if(channel->flush_state == libssh2_NB_state_idle) {
        LIBSSH2_PACKET *packet =
            _libssh2_list_first(&channel->session->packets);
        channel->flush_refund_bytes = 0;
        channel->flush_flush_bytes = 0;

        while(packet) {
            LIBSSH2_PACKET *next = _libssh2_list_next(&packet->node);
            unsigned char packet_type = packet->data[0];

            if(((packet_type == SSH_MSG_CHANNEL_DATA)
                 || (packet_type == SSH_MSG_CHANNEL_EXTENDED_DATA))
                && (_libssh2_ntohu32(packet->data + 1) == channel->local.id)) {
                /* It's our channel at least */
                long packet_stream_id =
                    (packet_type == SSH_MSG_CHANNEL_DATA) ? 0 :
                    _libssh2_ntohu32(packet->data + 5);
                if((streamid == LIBSSH2_CHANNEL_FLUSH_ALL)
                    || ((packet_type == SSH_MSG_CHANNEL_EXTENDED_DATA)
                        && ((streamid == LIBSSH2_CHANNEL_FLUSH_EXTENDED_DATA)
                            || (streamid == packet_stream_id)))
                    || ((packet_type == SSH_MSG_CHANNEL_DATA)
                        && (streamid == 0))) {
                    int bytes_to_flush = packet->data_len - packet->data_head;

                    _libssh2_debug(channel->session, LIBSSH2_TRACE_CONN,
                                   "Flushing %d bytes of data from stream "
                                   "%lu on channel %lu/%lu",
                                   bytes_to_flush, packet_stream_id,
                                   channel->local.id, channel->remote.id);

                    /* It's one of the streams we wanted to flush */
                    channel->flush_refund_bytes += packet->data_len - 13;
                    channel->flush_flush_bytes += bytes_to_flush;

                    LIBSSH2_FREE(channel->session, packet->data);

                    /* remove this packet from the parent's list */
                    _libssh2_list_remove(&packet->node);
                    LIBSSH2_FREE(channel->session, packet);
                }
            }
            packet = next;
        }

        channel->flush_state = libssh2_NB_state_created;
    }

    channel->read_avail -= channel->flush_flush_bytes;
    channel->remote.window_size -= channel->flush_flush_bytes;

    if(channel->flush_refund_bytes) {
        int rc;

        rc = _libssh2_channel_receive_window_adjust(channel,
                                                    channel->flush_refund_bytes,
                                                    1, NULL);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
    }

    channel->flush_state = libssh2_NB_state_idle;

    return channel->flush_flush_bytes;
}