_libssh2_channel_open(LIBSSH2_SESSION * session, const char *channel_type,
                      uint32_t channel_type_len,
                      uint32_t window_size,
                      uint32_t packet_size,
                      const unsigned char *message,
                      size_t message_len)
{
    static const unsigned char reply_codes[3] = {
        SSH_MSG_CHANNEL_OPEN_CONFIRMATION,
        SSH_MSG_CHANNEL_OPEN_FAILURE,
        0
    };
    unsigned char *s;
    int rc;

    if(session->open_state == libssh2_NB_state_idle) {
        session->open_channel = NULL;
        session->open_packet = NULL;
        session->open_data = NULL;
        /* 17 = packet_type(1) + channel_type_len(4) + sender_channel(4) +
         * window_size(4) + packet_size(4) */
        session->open_packet_len = channel_type_len + 17;
        session->open_local_channel = _libssh2_channel_nextid(session);

        /* Zero the whole thing out */
        memset(&session->open_packet_requirev_state, 0,
               sizeof(session->open_packet_requirev_state));

        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                       "Opening Channel - win %d pack %d", window_size,
                       packet_size);
        session->open_channel =
            LIBSSH2_CALLOC(session, sizeof(LIBSSH2_CHANNEL));
        if(!session->open_channel) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Unable to allocate space for channel data");
            return NULL;
        }
        session->open_channel->channel_type_len = channel_type_len;
        session->open_channel->channel_type =
            LIBSSH2_ALLOC(session, channel_type_len);
        if(!session->open_channel->channel_type) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Failed allocating memory for channel type name");
            LIBSSH2_FREE(session, session->open_channel);
            session->open_channel = NULL;
            return NULL;
        }
        memcpy(session->open_channel->channel_type, channel_type,
               channel_type_len);

        /* REMEMBER: local as in locally sourced */
        session->open_channel->local.id = session->open_local_channel;
        session->open_channel->remote.window_size = window_size;
        session->open_channel->remote.window_size_initial = window_size;
        session->open_channel->remote.packet_size = packet_size;
        session->open_channel->session = session;

        _libssh2_list_add(&session->channels,
                          &session->open_channel->node);

        s = session->open_packet =
            LIBSSH2_ALLOC(session, session->open_packet_len);
        if(!session->open_packet) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Unable to allocate temporary space for packet");
            goto channel_error;
        }
        *(s++) = SSH_MSG_CHANNEL_OPEN;
        _libssh2_store_str(&s, channel_type, channel_type_len);
        _libssh2_store_u32(&s, session->open_local_channel);
        _libssh2_store_u32(&s, window_size);
        _libssh2_store_u32(&s, packet_size);

        /* Do not copy the message */

        session->open_state = libssh2_NB_state_created;
    }

    if(session->open_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session,
                                     session->open_packet,
                                     session->open_packet_len,
                                     message, message_len);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, rc,
                           "Would block sending channel-open request");
            return NULL;
        }
        else if(rc) {
            _libssh2_error(session, rc,
                           "Unable to send channel-open request");
            goto channel_error;
        }

        session->open_state = libssh2_NB_state_sent;
    }

    if(session->open_state == libssh2_NB_state_sent) {
        rc = _libssh2_packet_requirev(session, reply_codes,
                                      &session->open_data,
                                      &session->open_data_len, 1,
                                      session->open_packet + 5 +
                                      channel_type_len, 4,
                                      &session->open_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
            return NULL;
        }
        else if(rc) {
            goto channel_error;
        }

        if(session->open_data[0] == SSH_MSG_CHANNEL_OPEN_CONFIRMATION) {
            session->open_channel->remote.id =
                _libssh2_ntohu32(session->open_data + 5);
            session->open_channel->local.window_size =
                _libssh2_ntohu32(session->open_data + 9);
            session->open_channel->local.window_size_initial =
                _libssh2_ntohu32(session->open_data + 9);
            session->open_channel->local.packet_size =
                _libssh2_ntohu32(session->open_data + 13);
            _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                           "Connection Established - ID: %lu/%lu win: %lu/%lu"
                           " pack: %lu/%lu",
                           session->open_channel->local.id,
                           session->open_channel->remote.id,
                           session->open_channel->local.window_size,
                           session->open_channel->remote.window_size,
                           session->open_channel->local.packet_size,
                           session->open_channel->remote.packet_size);
            LIBSSH2_FREE(session, session->open_packet);
            session->open_packet = NULL;
            LIBSSH2_FREE(session, session->open_data);
            session->open_data = NULL;

            session->open_state = libssh2_NB_state_idle;
            return session->open_channel;
        }

        if(session->open_data[0] == SSH_MSG_CHANNEL_OPEN_FAILURE) {
            unsigned int reason_code = _libssh2_ntohu32(session->open_data + 5);
            switch(reason_code) {
            case SSH_OPEN_ADMINISTRATIVELY_PROHIBITED:
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_FAILURE,
                               "Channel open failure (administratively prohibited)");
                break;
            case SSH_OPEN_CONNECT_FAILED:
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_FAILURE,
                               "Channel open failure (connect failed)");
                break;
            case SSH_OPEN_UNKNOWN_CHANNELTYPE:
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_FAILURE,
                               "Channel open failure (unknown channel type)");
                break;
            case SSH_OPEN_RESOURCE_SHORTAGE:
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_FAILURE,
                               "Channel open failure (resource shortage)");
                break;
            default:
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_FAILURE,
                               "Channel open failure");
            }
        }
    }

  channel_error:

    if(session->open_data) {
        LIBSSH2_FREE(session, session->open_data);
        session->open_data = NULL;
    }
    if(session->open_packet) {
        LIBSSH2_FREE(session, session->open_packet);
        session->open_packet = NULL;
    }
    if(session->open_channel) {
        unsigned char channel_id[4];
        LIBSSH2_FREE(session, session->open_channel->channel_type);

        _libssh2_list_remove(&session->open_channel->node);

        /* Clear out packets meant for this channel */
        _libssh2_htonu32(channel_id, session->open_channel->local.id);
        while((_libssh2_packet_ask(session, SSH_MSG_CHANNEL_DATA,
                                    &session->open_data,
                                    &session->open_data_len, 1,
                                    channel_id, 4) >= 0)
               ||
               (_libssh2_packet_ask(session, SSH_MSG_CHANNEL_EXTENDED_DATA,
                                    &session->open_data,
                                    &session->open_data_len, 1,
                                    channel_id, 4) >= 0)) {
            LIBSSH2_FREE(session, session->open_data);
            session->open_data = NULL;
        }

        LIBSSH2_FREE(session, session->open_channel);
        session->open_channel = NULL;
    }

    session->open_state = libssh2_NB_state_idle;
    return NULL;
}