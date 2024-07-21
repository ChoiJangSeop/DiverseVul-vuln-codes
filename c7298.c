channel_x11_req(LIBSSH2_CHANNEL *channel, int single_connection,
                const char *auth_proto, const char *auth_cookie,
                int screen_number)
{
    LIBSSH2_SESSION *session = channel->session;
    unsigned char *s;
    static const unsigned char reply_codes[3] =
        { SSH_MSG_CHANNEL_SUCCESS, SSH_MSG_CHANNEL_FAILURE, 0 };
    size_t proto_len =
        auth_proto ? strlen(auth_proto) : (sizeof("MIT-MAGIC-COOKIE-1") - 1);
    size_t cookie_len =
        auth_cookie ? strlen(auth_cookie) : LIBSSH2_X11_RANDOM_COOKIE_LEN;
    int rc;

    if(channel->reqX11_state == libssh2_NB_state_idle) {
        /* 30 = packet_type(1) + channel(4) + x11_req_len(4) + "x11-req"(7) +
         * want_reply(1) + single_cnx(1) + proto_len(4) + cookie_len(4) +
         * screen_num(4) */
        channel->reqX11_packet_len = proto_len + cookie_len + 30;

        /* Zero the whole thing out */
        memset(&channel->reqX11_packet_requirev_state, 0,
               sizeof(channel->reqX11_packet_requirev_state));

        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                       "Requesting x11-req for channel %lu/%lu: single=%d "
                       "proto=%s cookie=%s screen=%d",
                       channel->local.id, channel->remote.id,
                       single_connection,
                       auth_proto ? auth_proto : "MIT-MAGIC-COOKIE-1",
                       auth_cookie ? auth_cookie : "<random>", screen_number);

        s = channel->reqX11_packet =
            LIBSSH2_ALLOC(session, channel->reqX11_packet_len);
        if(!channel->reqX11_packet) {
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Unable to allocate memory for pty-request");
        }

        *(s++) = SSH_MSG_CHANNEL_REQUEST;
        _libssh2_store_u32(&s, channel->remote.id);
        _libssh2_store_str(&s, "x11-req", sizeof("x11-req") - 1);

        *(s++) = 0x01;          /* want_reply */
        *(s++) = single_connection ? 0x01 : 0x00;

        _libssh2_store_str(&s, auth_proto ? auth_proto : "MIT-MAGIC-COOKIE-1",
                           proto_len);

        _libssh2_store_u32(&s, cookie_len);
        if(auth_cookie) {
            memcpy(s, auth_cookie, cookie_len);
        }
        else {
            int i;
            /* note: the extra +1 below is necessary since the sprintf()
               loop will always write 3 bytes so the last one will write
               the trailing zero at the LIBSSH2_X11_RANDOM_COOKIE_LEN/2
               border */
            unsigned char buffer[(LIBSSH2_X11_RANDOM_COOKIE_LEN / 2) + 1];

            _libssh2_random(buffer, LIBSSH2_X11_RANDOM_COOKIE_LEN / 2);
            for(i = 0; i < (LIBSSH2_X11_RANDOM_COOKIE_LEN / 2); i++) {
                snprintf((char *)&s[i*2], 3, "%02X%c", buffer[i], '\0');
            }
        }
        s += cookie_len;

        _libssh2_store_u32(&s, screen_number);
        channel->reqX11_state = libssh2_NB_state_created;
    }

    if(channel->reqX11_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, channel->reqX11_packet,
                                     channel->reqX11_packet_len,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, rc,
                           "Would block sending X11-req packet");
            return rc;
        }
        if(rc) {
            LIBSSH2_FREE(session, channel->reqX11_packet);
            channel->reqX11_packet = NULL;
            channel->reqX11_state = libssh2_NB_state_idle;
            return _libssh2_error(session, rc,
                                  "Unable to send x11-req packet");
        }
        LIBSSH2_FREE(session, channel->reqX11_packet);
        channel->reqX11_packet = NULL;

        _libssh2_htonu32(channel->reqX11_local_channel, channel->local.id);

        channel->reqX11_state = libssh2_NB_state_sent;
    }

    if(channel->reqX11_state == libssh2_NB_state_sent) {
        size_t data_len;
        unsigned char *data;
        unsigned char code;

        rc = _libssh2_packet_requirev(session, reply_codes, &data, &data_len,
                                      1, channel->reqX11_local_channel, 4,
                                      &channel->reqX11_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return rc;
        }
        else if(rc) {
            channel->reqX11_state = libssh2_NB_state_idle;
            return _libssh2_error(session, rc,
                                  "waiting for x11-req response packet");
        }

        code = data[0];
        LIBSSH2_FREE(session, data);
        channel->reqX11_state = libssh2_NB_state_idle;

        if(code == SSH_MSG_CHANNEL_SUCCESS)
            return 0;
    }

    return _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED,
                          "Unable to complete request for channel x11-req");
}