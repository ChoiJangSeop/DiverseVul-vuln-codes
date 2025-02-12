_libssh2_packet_add(LIBSSH2_SESSION * session, unsigned char *data,
                    size_t datalen, int macstate)
{
    int rc = 0;
    char *message = NULL;
    char *language = NULL;
    size_t message_len = 0;
    size_t language_len = 0;
    LIBSSH2_CHANNEL *channelp = NULL;
    size_t data_head = 0;
    unsigned char msg = data[0];

    switch(session->packAdd_state) {
    case libssh2_NB_state_idle:
        _libssh2_debug(session, LIBSSH2_TRACE_TRANS,
                       "Packet type %d received, length=%d",
                       (int) msg, (int) datalen);

        if((macstate == LIBSSH2_MAC_INVALID) &&
            (!session->macerror ||
             LIBSSH2_MACERROR(session, (char *) data, datalen))) {
            /* Bad MAC input, but no callback set or non-zero return from the
               callback */

            LIBSSH2_FREE(session, data);
            return _libssh2_error(session, LIBSSH2_ERROR_INVALID_MAC,
                                  "Invalid MAC received");
        }
        session->packAdd_state = libssh2_NB_state_allocated;
        break;
    case libssh2_NB_state_jump1:
        goto libssh2_packet_add_jump_point1;
    case libssh2_NB_state_jump2:
        goto libssh2_packet_add_jump_point2;
    case libssh2_NB_state_jump3:
        goto libssh2_packet_add_jump_point3;
    case libssh2_NB_state_jump4:
        goto libssh2_packet_add_jump_point4;
    case libssh2_NB_state_jump5:
        goto libssh2_packet_add_jump_point5;
    default: /* nothing to do */
        break;
    }

    if(session->packAdd_state == libssh2_NB_state_allocated) {
        /* A couple exceptions to the packet adding rule: */
        switch(msg) {

            /*
              byte      SSH_MSG_DISCONNECT
              uint32    reason code
              string    description in ISO-10646 UTF-8 encoding [RFC3629]
              string    language tag [RFC3066]
            */

        case SSH_MSG_DISCONNECT:
            if(datalen >= 5) {
                size_t reason = _libssh2_ntohu32(data + 1);

                if(datalen >= 9) {
                    message_len = _libssh2_ntohu32(data + 5);

                    if(message_len < datalen-13) {
                        /* 9 = packet_type(1) + reason(4) + message_len(4) */
                        message = (char *) data + 9;

                        language_len =
                            _libssh2_ntohu32(data + 9 + message_len);
                        language = (char *) data + 9 + message_len + 4;

                        if(language_len > (datalen-13-message_len)) {
                            /* bad input, clear info */
                            language = message = NULL;
                            language_len = message_len = 0;
                        }
                    }
                    else
                        /* bad size, clear it */
                        message_len = 0;
                }
                if(session->ssh_msg_disconnect) {
                    LIBSSH2_DISCONNECT(session, reason, message,
                                       message_len, language, language_len);
                }
                _libssh2_debug(session, LIBSSH2_TRACE_TRANS,
                               "Disconnect(%d): %s(%s)", reason,
                               message, language);
            }

            LIBSSH2_FREE(session, data);
            session->socket_state = LIBSSH2_SOCKET_DISCONNECTED;
            session->packAdd_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_DISCONNECT,
                                  "socket disconnect");
            /*
              byte      SSH_MSG_IGNORE
              string    data
            */

        case SSH_MSG_IGNORE:
            if(datalen >= 2) {
                if(session->ssh_msg_ignore) {
                    LIBSSH2_IGNORE(session, (char *) data + 1, datalen - 1);
                }
            }
            else if(session->ssh_msg_ignore) {
                LIBSSH2_IGNORE(session, "", 0);
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_DEBUG
              boolean   always_display
              string    message in ISO-10646 UTF-8 encoding [RFC3629]
              string    language tag [RFC3066]
            */

        case SSH_MSG_DEBUG:
            if(datalen >= 2) {
                int always_display = data[1];

                if(datalen >= 6) {
                    message_len = _libssh2_ntohu32(data + 2);

                    if(message_len <= (datalen - 10)) {
                        /* 6 = packet_type(1) + display(1) + message_len(4) */
                        message = (char *) data + 6;
                        language_len = _libssh2_ntohu32(data + 6 +
                                                        message_len);

                        if(language_len <= (datalen - 10 - message_len))
                            language = (char *) data + 10 + message_len;
                    }
                }

                if(session->ssh_msg_debug) {
                    LIBSSH2_DEBUG(session, always_display, message,
                                  message_len, language, language_len);
                }
            }
            /*
             * _libssh2_debug will actually truncate this for us so
             * that it's not an inordinate about of data
             */
            _libssh2_debug(session, LIBSSH2_TRACE_TRANS,
                           "Debug Packet: %s", message);
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_GLOBAL_REQUEST
              string    request name in US-ASCII only
              boolean   want reply
              ....      request-specific data follows
            */

        case SSH_MSG_GLOBAL_REQUEST:
            if(datalen >= 5) {
                uint32_t len = 0;
                unsigned char want_reply = 0;
                len = _libssh2_ntohu32(data + 1);
                if(datalen >= (6 + len)) {
                    want_reply = data[5 + len];
                    _libssh2_debug(session,
                                   LIBSSH2_TRACE_CONN,
                                   "Received global request type %.*s (wr %X)",
                                   len, data + 5, want_reply);
                }


                if(want_reply) {
                    static const unsigned char packet =
                        SSH_MSG_REQUEST_FAILURE;
                  libssh2_packet_add_jump_point5:
                    session->packAdd_state = libssh2_NB_state_jump5;
                    rc = _libssh2_transport_send(session, &packet, 1, NULL, 0);
                    if(rc == LIBSSH2_ERROR_EAGAIN)
                        return rc;
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_EXTENDED_DATA
              uint32    recipient channel
              uint32    data_type_code
              string    data
            */

        case SSH_MSG_CHANNEL_EXTENDED_DATA:
            /* streamid(4) */
            data_head += 4;

            /* fall-through */

            /*
              byte      SSH_MSG_CHANNEL_DATA
              uint32    recipient channel
              string    data
            */

        case SSH_MSG_CHANNEL_DATA:
            /* packet_type(1) + channelno(4) + datalen(4) */
            data_head += 9;

            if(datalen >= data_head)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));

            if(!channelp) {
                _libssh2_error(session, LIBSSH2_ERROR_CHANNEL_UNKNOWN,
                               "Packet received for unknown channel");
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
#ifdef LIBSSH2DEBUG
            {
                uint32_t stream_id = 0;
                if(msg == SSH_MSG_CHANNEL_EXTENDED_DATA)
                    stream_id = _libssh2_ntohu32(data + 5);

                _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                               "%d bytes packet_add() for %lu/%lu/%lu",
                               (int) (datalen - data_head),
                               channelp->local.id,
                               channelp->remote.id,
                               stream_id);
            }
#endif
            if((channelp->remote.extended_data_ignore_mode ==
                 LIBSSH2_CHANNEL_EXTENDED_DATA_IGNORE) &&
                (msg == SSH_MSG_CHANNEL_EXTENDED_DATA)) {
                /* Pretend we didn't receive this */
                LIBSSH2_FREE(session, data);

                _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                               "Ignoring extended data and refunding %d bytes",
                               (int) (datalen - 13));
                if(channelp->read_avail + datalen - data_head >=
                    channelp->remote.window_size)
                    datalen = channelp->remote.window_size -
                        channelp->read_avail + data_head;

                channelp->remote.window_size -= datalen - data_head;
                _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                               "shrinking window size by %lu bytes to %lu, "
                               "read_avail %lu",
                               datalen - data_head,
                               channelp->remote.window_size,
                               channelp->read_avail);

                session->packAdd_channelp = channelp;

                /* Adjust the window based on the block we just freed */
              libssh2_packet_add_jump_point1:
                session->packAdd_state = libssh2_NB_state_jump1;
                rc = _libssh2_channel_receive_window_adjust(session->
                                                            packAdd_channelp,
                                                            datalen - 13,
                                                            1, NULL);
                if(rc == LIBSSH2_ERROR_EAGAIN)
                    return rc;

                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }

            /*
             * REMEMBER! remote means remote as source of data,
             * NOT remote window!
             */
            if(channelp->remote.packet_size < (datalen - data_head)) {
                /*
                 * Spec says we MAY ignore bytes sent beyond
                 * packet_size
                 */
                _libssh2_error(session,
                               LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED,
                               "Packet contains more data than we offered"
                               " to receive, truncating");
                datalen = channelp->remote.packet_size + data_head;
            }
            if(channelp->remote.window_size <= channelp->read_avail) {
                /*
                 * Spec says we MAY ignore bytes sent beyond
                 * window_size
                 */
                _libssh2_error(session,
                               LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED,
                               "The current receive window is full,"
                               " data ignored");
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
            /* Reset EOF status */
            channelp->remote.eof = 0;

            if(channelp->read_avail + datalen - data_head >
                channelp->remote.window_size) {
                _libssh2_error(session,
                               LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED,
                               "Remote sent more data than current "
                               "window allows, truncating");
                datalen = channelp->remote.window_size -
                    channelp->read_avail + data_head;
            }

            /* Update the read_avail counter. The window size will be
             * updated once the data is actually read from the queue
             * from an upper layer */
            channelp->read_avail += datalen - data_head;

            _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                           "increasing read_avail by %lu bytes to %lu/%lu",
                           (long)(datalen - data_head),
                           (long)channelp->read_avail,
                           (long)channelp->remote.window_size);

            break;

            /*
              byte      SSH_MSG_CHANNEL_EOF
              uint32    recipient channel
            */

        case SSH_MSG_CHANNEL_EOF:
            if(datalen >= 5)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
            if(!channelp)
                /* We may have freed already, just quietly ignore this... */
                ;
            else {
                _libssh2_debug(session,
                               LIBSSH2_TRACE_CONN,
                               "EOF received for channel %lu/%lu",
                               channelp->local.id,
                               channelp->remote.id);
                channelp->remote.eof = 1;
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_REQUEST
              uint32    recipient channel
              string    request type in US-ASCII characters only
              boolean   want reply
              ....      type-specific data follows
            */

        case SSH_MSG_CHANNEL_REQUEST:
            if(datalen >= 9) {
                uint32_t channel = _libssh2_ntohu32(data + 1);
                uint32_t len = _libssh2_ntohu32(data + 5);
                unsigned char want_reply = 1;

                if((len + 9) < datalen)
                    want_reply = data[len + 9];

                _libssh2_debug(session,
                               LIBSSH2_TRACE_CONN,
                               "Channel %d received request type %.*s (wr %X)",
                               channel, len, data + 9, want_reply);

                if(len == sizeof("exit-status") - 1
                    && (sizeof("exit-status") - 1 + 9) <= datalen
                    && !memcmp("exit-status", data + 9,
                               sizeof("exit-status") - 1)) {

                    /* we've got "exit-status" packet. Set the session value */
                    if(datalen >= 20)
                        channelp =
                            _libssh2_channel_locate(session, channel);

                    if(channelp && (sizeof("exit-status") + 13) <= datalen) {
                        channelp->exit_status =
                            _libssh2_ntohu32(data + 9 + sizeof("exit-status"));
                        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                                       "Exit status %lu received for "
                                       "channel %lu/%lu",
                                       channelp->exit_status,
                                       channelp->local.id,
                                       channelp->remote.id);
                    }

                }
                else if(len == sizeof("exit-signal") - 1
                         && (sizeof("exit-signal") - 1 + 9) <= datalen
                         && !memcmp("exit-signal", data + 9,
                                    sizeof("exit-signal") - 1)) {
                    /* command terminated due to signal */
                    if(datalen >= 20)
                        channelp = _libssh2_channel_locate(session, channel);

                    if(channelp && (sizeof("exit-signal") + 13) <= datalen) {
                        /* set signal name (without SIG prefix) */
                        uint32_t namelen =
                            _libssh2_ntohu32(data + 9 + sizeof("exit-signal"));

                        if(namelen <= UINT_MAX - 1) {
                            channelp->exit_signal =
                                LIBSSH2_ALLOC(session, namelen + 1);
                        }
                        else {
                            channelp->exit_signal = NULL;
                        }

                        if(!channelp->exit_signal)
                            rc = _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                                "memory for signal name");
                        else if((sizeof("exit-signal") + 13 + namelen <=
                                 datalen)) {
                            memcpy(channelp->exit_signal,
                                   data + 13 + sizeof("exit-signal"), namelen);
                            channelp->exit_signal[namelen] = '\0';
                            /* TODO: save error message and language tag */
                            _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                                           "Exit signal %s received for "
                                           "channel %lu/%lu",
                                           channelp->exit_signal,
                                           channelp->local.id,
                                           channelp->remote.id);
                        }
                    }
                }


                if(want_reply) {
                    unsigned char packet[5];
                  libssh2_packet_add_jump_point4:
                    session->packAdd_state = libssh2_NB_state_jump4;
                    packet[0] = SSH_MSG_CHANNEL_FAILURE;
                    memcpy(&packet[1], data + 1, 4);
                    rc = _libssh2_transport_send(session, packet, 5, NULL, 0);
                    if(rc == LIBSSH2_ERROR_EAGAIN)
                        return rc;
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return rc;

            /*
              byte      SSH_MSG_CHANNEL_CLOSE
              uint32    recipient channel
            */

        case SSH_MSG_CHANNEL_CLOSE:
            if(datalen >= 5)
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
            if(!channelp) {
                /* We may have freed already, just quietly ignore this... */
                LIBSSH2_FREE(session, data);
                session->packAdd_state = libssh2_NB_state_idle;
                return 0;
            }
            _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                           "Close received for channel %lu/%lu",
                           channelp->local.id,
                           channelp->remote.id);

            channelp->remote.close = 1;
            channelp->remote.eof = 1;

            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;

            /*
              byte      SSH_MSG_CHANNEL_OPEN
              string    "session"
              uint32    sender channel
              uint32    initial window size
              uint32    maximum packet size
            */

        case SSH_MSG_CHANNEL_OPEN:
            if(datalen < 17)
                ;
            else if((datalen >= (sizeof("forwarded-tcpip") + 4)) &&
                     ((sizeof("forwarded-tcpip") - 1) ==
                      _libssh2_ntohu32(data + 1))
                     &&
                     (memcmp(data + 5, "forwarded-tcpip",
                             sizeof("forwarded-tcpip") - 1) == 0)) {

                /* init the state struct */
                memset(&session->packAdd_Qlstn_state, 0,
                       sizeof(session->packAdd_Qlstn_state));

              libssh2_packet_add_jump_point2:
                session->packAdd_state = libssh2_NB_state_jump2;
                rc = packet_queue_listener(session, data, datalen,
                                           &session->packAdd_Qlstn_state);
            }
            else if((datalen >= (sizeof("x11") + 4)) &&
                     ((sizeof("x11") - 1) == _libssh2_ntohu32(data + 1)) &&
                     (memcmp(data + 5, "x11", sizeof("x11") - 1) == 0)) {

                /* init the state struct */
                memset(&session->packAdd_x11open_state, 0,
                       sizeof(session->packAdd_x11open_state));

              libssh2_packet_add_jump_point3:
                session->packAdd_state = libssh2_NB_state_jump3;
                rc = packet_x11_open(session, data, datalen,
                                     &session->packAdd_x11open_state);
            }
            if(rc == LIBSSH2_ERROR_EAGAIN)
                return rc;

            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return rc;

            /*
              byte      SSH_MSG_CHANNEL_WINDOW_ADJUST
              uint32    recipient channel
              uint32    bytes to add
            */
        case SSH_MSG_CHANNEL_WINDOW_ADJUST:
            if(datalen < 9)
                ;
            else {
                uint32_t bytestoadd = _libssh2_ntohu32(data + 5);
                channelp =
                    _libssh2_channel_locate(session,
                                            _libssh2_ntohu32(data + 1));
                if(channelp) {
                    channelp->local.window_size += bytestoadd;

                    _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                                   "Window adjust for channel %lu/%lu, "
                                   "adding %lu bytes, new window_size=%lu",
                                   channelp->local.id,
                                   channelp->remote.id,
                                   bytestoadd,
                                   channelp->local.window_size);
                }
            }
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return 0;
        default:
            break;
        }

        session->packAdd_state = libssh2_NB_state_sent;
    }

    if(session->packAdd_state == libssh2_NB_state_sent) {
        LIBSSH2_PACKET *packetp =
            LIBSSH2_ALLOC(session, sizeof(LIBSSH2_PACKET));
        if(!packetp) {
            _libssh2_debug(session, LIBSSH2_ERROR_ALLOC,
                           "memory for packet");
            LIBSSH2_FREE(session, data);
            session->packAdd_state = libssh2_NB_state_idle;
            return LIBSSH2_ERROR_ALLOC;
        }
        packetp->data = data;
        packetp->data_len = datalen;
        packetp->data_head = data_head;

        _libssh2_list_add(&session->packets, &packetp->node);

        session->packAdd_state = libssh2_NB_state_sent1;
    }

    if((msg == SSH_MSG_KEXINIT &&
         !(session->state & LIBSSH2_STATE_EXCHANGING_KEYS)) ||
        (session->packAdd_state == libssh2_NB_state_sent2)) {
        if(session->packAdd_state == libssh2_NB_state_sent1) {
            /*
             * Remote wants new keys
             * Well, it's already in the brigade,
             * let's just call back into ourselves
             */
            _libssh2_debug(session, LIBSSH2_TRACE_TRANS, "Renegotiating Keys");

            session->packAdd_state = libssh2_NB_state_sent2;
        }

        /*
         * The KEXINIT message has been added to the queue.  The packAdd and
         * readPack states need to be reset because _libssh2_kex_exchange
         * (eventually) calls upon _libssh2_transport_read to read the rest of
         * the key exchange conversation.
         */
        session->readPack_state = libssh2_NB_state_idle;
        session->packet.total_num = 0;
        session->packAdd_state = libssh2_NB_state_idle;
        session->fullpacket_state = libssh2_NB_state_idle;

        memset(&session->startup_key_state, 0, sizeof(key_exchange_state_t));

        /*
         * If there was a key reexchange failure, let's just hope we didn't
         * send NEWKEYS yet, otherwise remote will drop us like a rock
         */
        rc = _libssh2_kex_exchange(session, 1, &session->startup_key_state);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
    }

    session->packAdd_state = libssh2_NB_state_idle;
    return 0;
}