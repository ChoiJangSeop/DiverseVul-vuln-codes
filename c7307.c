_libssh2_channel_process_startup(LIBSSH2_CHANNEL *channel,
                                 const char *request, size_t request_len,
                                 const char *message, size_t message_len)
{
    LIBSSH2_SESSION *session = channel->session;
    unsigned char *s;
    static const unsigned char reply_codes[3] =
        { SSH_MSG_CHANNEL_SUCCESS, SSH_MSG_CHANNEL_FAILURE, 0 };
    int rc;

    if(channel->process_state == libssh2_NB_state_end) {
        return _libssh2_error(session, LIBSSH2_ERROR_BAD_USE,
                              "Channel can not be reused");
    }

    if(channel->process_state == libssh2_NB_state_idle) {
        /* 10 = packet_type(1) + channel(4) + request_len(4) + want_reply(1) */
        channel->process_packet_len = request_len + 10;

        /* Zero the whole thing out */
        memset(&channel->process_packet_requirev_state, 0,
               sizeof(channel->process_packet_requirev_state));

        if(message)
            channel->process_packet_len += + 4;

        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                       "starting request(%s) on channel %lu/%lu, message=%s",
                       request, channel->local.id, channel->remote.id,
                       message ? message : "<null>");
        s = channel->process_packet =
            LIBSSH2_ALLOC(session, channel->process_packet_len);
        if(!channel->process_packet)
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Unable to allocate memory "
                                  "for channel-process request");

        *(s++) = SSH_MSG_CHANNEL_REQUEST;
        _libssh2_store_u32(&s, channel->remote.id);
        _libssh2_store_str(&s, request, request_len);
        *(s++) = 0x01;

        if(message)
            _libssh2_store_u32(&s, message_len);

        channel->process_state = libssh2_NB_state_created;
    }

    if(channel->process_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session,
                                     channel->process_packet,
                                     channel->process_packet_len,
                                     (unsigned char *)message, message_len);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, rc,
                           "Would block sending channel request");
            return rc;
        }
        else if(rc) {
            LIBSSH2_FREE(session, channel->process_packet);
            channel->process_packet = NULL;
            channel->process_state = libssh2_NB_state_end;
            return _libssh2_error(session, rc,
                                  "Unable to send channel request");
        }
        LIBSSH2_FREE(session, channel->process_packet);
        channel->process_packet = NULL;

        _libssh2_htonu32(channel->process_local_channel, channel->local.id);

        channel->process_state = libssh2_NB_state_sent;
    }

    if(channel->process_state == libssh2_NB_state_sent) {
        unsigned char *data;
        size_t data_len;
        unsigned char code;
        rc = _libssh2_packet_requirev(session, reply_codes, &data, &data_len,
                                      1, channel->process_local_channel, 4,
                                      &channel->process_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return rc;
        }
        else if(rc) {
            channel->process_state = libssh2_NB_state_end;
            return _libssh2_error(session, rc,
                                  "Failed waiting for channel success");
        }

        code = data[0];
        LIBSSH2_FREE(session, data);
        channel->process_state = libssh2_NB_state_end;

        if(code == SSH_MSG_CHANNEL_SUCCESS)
            return 0;
    }

    return _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED,
                          "Unable to complete request for "
                          "channel-process-startup");
}