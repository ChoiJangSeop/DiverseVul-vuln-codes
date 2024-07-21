static int channel_request_pty(LIBSSH2_CHANNEL *channel,
                               const char *term, unsigned int term_len,
                               const char *modes, unsigned int modes_len,
                               int width, int height,
                               int width_px, int height_px)
{
    LIBSSH2_SESSION *session = channel->session;
    unsigned char *s;
    static const unsigned char reply_codes[3] =
        { SSH_MSG_CHANNEL_SUCCESS, SSH_MSG_CHANNEL_FAILURE, 0 };
    int rc;

    if(channel->reqPTY_state == libssh2_NB_state_idle) {
        /* 41 = packet_type(1) + channel(4) + pty_req_len(4) + "pty_req"(7) +
         * want_reply(1) + term_len(4) + width(4) + height(4) + width_px(4) +
         * height_px(4) + modes_len(4) */
        if(term_len + modes_len > 256) {
            return _libssh2_error(session, LIBSSH2_ERROR_INVAL,
                                  "term + mode lengths too large");
        }

        channel->reqPTY_packet_len = term_len + modes_len + 41;

        /* Zero the whole thing out */
        memset(&channel->reqPTY_packet_requirev_state, 0,
               sizeof(channel->reqPTY_packet_requirev_state));

        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                       "Allocating tty on channel %lu/%lu", channel->local.id,
                       channel->remote.id);

        s = channel->reqPTY_packet;

        *(s++) = SSH_MSG_CHANNEL_REQUEST;
        _libssh2_store_u32(&s, channel->remote.id);
        _libssh2_store_str(&s, (char *)"pty-req", sizeof("pty-req") - 1);

        *(s++) = 0x01;

        _libssh2_store_str(&s, term, term_len);
        _libssh2_store_u32(&s, width);
        _libssh2_store_u32(&s, height);
        _libssh2_store_u32(&s, width_px);
        _libssh2_store_u32(&s, height_px);
        _libssh2_store_str(&s, modes, modes_len);

        channel->reqPTY_state = libssh2_NB_state_created;
    }

    if(channel->reqPTY_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, channel->reqPTY_packet,
                                     channel->reqPTY_packet_len,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, rc,
                           "Would block sending pty request");
            return rc;
        }
        else if(rc) {
            channel->reqPTY_state = libssh2_NB_state_idle;
            return _libssh2_error(session, rc,
                                  "Unable to send pty-request packet");
        }
        _libssh2_htonu32(channel->reqPTY_local_channel, channel->local.id);

        channel->reqPTY_state = libssh2_NB_state_sent;
    }

    if(channel->reqPTY_state == libssh2_NB_state_sent) {
        unsigned char *data;
        size_t data_len;
        unsigned char code;
        rc = _libssh2_packet_requirev(session, reply_codes, &data, &data_len,
                                      1, channel->reqPTY_local_channel, 4,
                                      &channel->reqPTY_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return rc;
        }
        else if(rc) {
            channel->reqPTY_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                                  "Failed to require the PTY package");
        }

        code = data[0];

        LIBSSH2_FREE(session, data);
        channel->reqPTY_state = libssh2_NB_state_idle;

        if(code == SSH_MSG_CHANNEL_SUCCESS)
            return 0;
    }

    return _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED,
                          "Unable to complete request for channel request-pty");
}