libssh2_channel_window_read_ex(LIBSSH2_CHANNEL *channel,
                               unsigned long *read_avail,
                               unsigned long *window_size_initial)
{
    if(!channel)
        return 0; /* no channel, no window! */

    if(window_size_initial) {
        *window_size_initial = channel->remote.window_size_initial;
    }

    if(read_avail) {
        size_t bytes_queued = 0;
        LIBSSH2_PACKET *packet =
            _libssh2_list_first(&channel->session->packets);

        while(packet) {
            unsigned char packet_type = packet->data[0];

            if(((packet_type == SSH_MSG_CHANNEL_DATA)
                 || (packet_type == SSH_MSG_CHANNEL_EXTENDED_DATA))
                && (_libssh2_ntohu32(packet->data + 1) == channel->local.id)) {
                bytes_queued += packet->data_len - packet->data_head;
            }

            packet = _libssh2_list_next(&packet->node);
        }

        *read_avail = bytes_queued;
    }

    return channel->remote.window_size;
}