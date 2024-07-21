_libssh2_channel_packet_data_len(LIBSSH2_CHANNEL * channel, int stream_id)
{
    LIBSSH2_SESSION *session = channel->session;
    LIBSSH2_PACKET *read_packet;
    uint32_t read_local_id;

    read_packet = _libssh2_list_first(&session->packets);
    if(read_packet == NULL)
        return 0;

    while(read_packet) {
        read_local_id = _libssh2_ntohu32(read_packet->data + 1);

        /*
         * Either we asked for a specific extended data stream
         * (and data was available),
         * or the standard stream (and data was available),
         * or the standard stream with extended_data_merge
         * enabled and data was available
         */
        if((stream_id
             && (read_packet->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
             && (channel->local.id == read_local_id)
             && (stream_id == (int) _libssh2_ntohu32(read_packet->data + 5)))
            ||
            (!stream_id
             && (read_packet->data[0] == SSH_MSG_CHANNEL_DATA)
             && (channel->local.id == read_local_id))
            ||
            (!stream_id
             && (read_packet->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA)
             && (channel->local.id == read_local_id)
             && (channel->remote.extended_data_ignore_mode
                 == LIBSSH2_CHANNEL_EXTENDED_DATA_MERGE))) {
            return (read_packet->data_len - read_packet->data_head);
        }
        read_packet = _libssh2_list_next(&read_packet->node);
    }

    return 0;
}