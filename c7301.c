libssh2_channel_eof(LIBSSH2_CHANNEL * channel)
{
    LIBSSH2_SESSION *session;
    LIBSSH2_PACKET *packet;

    if(!channel)
        return LIBSSH2_ERROR_BAD_USE;

    session = channel->session;
    packet = _libssh2_list_first(&session->packets);

    while(packet) {
        if(((packet->data[0] == SSH_MSG_CHANNEL_DATA)
             || (packet->data[0] == SSH_MSG_CHANNEL_EXTENDED_DATA))
            && (channel->local.id == _libssh2_ntohu32(packet->data + 1))) {
            /* There's data waiting to be read yet, mask the EOF status */
            return 0;
        }
        packet = _libssh2_list_next(&packet->node);
    }

    return channel->remote.eof;
}