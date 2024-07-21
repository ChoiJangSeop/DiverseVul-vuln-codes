channel_forward_listen(LIBSSH2_SESSION * session, const char *host,
                       int port, int *bound_port, int queue_maxsize)
{
    unsigned char *s;
    static const unsigned char reply_codes[3] =
        { SSH_MSG_REQUEST_SUCCESS, SSH_MSG_REQUEST_FAILURE, 0 };
    int rc;

    if(!host)
        host = "0.0.0.0";

    if(session->fwdLstn_state == libssh2_NB_state_idle) {
        session->fwdLstn_host_len = strlen(host);
        /* 14 = packet_type(1) + request_len(4) + want_replay(1) + host_len(4)
           + port(4) */
        session->fwdLstn_packet_len =
            session->fwdLstn_host_len + (sizeof("tcpip-forward") - 1) + 14;

        /* Zero the whole thing out */
        memset(&session->fwdLstn_packet_requirev_state, 0,
               sizeof(session->fwdLstn_packet_requirev_state));

        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                       "Requesting tcpip-forward session for %s:%d", host,
                       port);

        s = session->fwdLstn_packet =
            LIBSSH2_ALLOC(session, session->fwdLstn_packet_len);
        if(!session->fwdLstn_packet) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Unable to allocate memory for setenv packet");
            return NULL;
        }

        *(s++) = SSH_MSG_GLOBAL_REQUEST;
        _libssh2_store_str(&s, "tcpip-forward", sizeof("tcpip-forward") - 1);
        *(s++) = 0x01;          /* want_reply */

        _libssh2_store_str(&s, host, session->fwdLstn_host_len);
        _libssh2_store_u32(&s, port);

        session->fwdLstn_state = libssh2_NB_state_created;
    }

    if(session->fwdLstn_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session,
                                     session->fwdLstn_packet,
                                     session->fwdLstn_packet_len,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                           "Would block sending global-request packet for "
                           "forward listen request");
            return NULL;
        }
        else if(rc) {
            _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                           "Unable to send global-request packet for forward "
                           "listen request");
            LIBSSH2_FREE(session, session->fwdLstn_packet);
            session->fwdLstn_packet = NULL;
            session->fwdLstn_state = libssh2_NB_state_idle;
            return NULL;
        }
        LIBSSH2_FREE(session, session->fwdLstn_packet);
        session->fwdLstn_packet = NULL;

        session->fwdLstn_state = libssh2_NB_state_sent;
    }

    if(session->fwdLstn_state == libssh2_NB_state_sent) {
        unsigned char *data;
        size_t data_len;
        rc = _libssh2_packet_requirev(session, reply_codes, &data, &data_len,
                                      0, NULL, 0,
                                      &session->fwdLstn_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
            return NULL;
        }
        else if(rc) {
            _libssh2_error(session, LIBSSH2_ERROR_PROTO, "Unknown");
            session->fwdLstn_state = libssh2_NB_state_idle;
            return NULL;
        }

        if(data[0] == SSH_MSG_REQUEST_SUCCESS) {
            LIBSSH2_LISTENER *listener;

            listener = LIBSSH2_CALLOC(session, sizeof(LIBSSH2_LISTENER));
            if(!listener)
                _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                               "Unable to allocate memory for listener queue");
            else {
                listener->host =
                    LIBSSH2_ALLOC(session, session->fwdLstn_host_len + 1);
                if(!listener->host) {
                    _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                   "Unable to allocate memory for listener queue");
                    LIBSSH2_FREE(session, listener);
                    listener = NULL;
                }
                else {
                    listener->session = session;
                    memcpy(listener->host, host, session->fwdLstn_host_len);
                    listener->host[session->fwdLstn_host_len] = 0;
                    if(data_len >= 5 && !port) {
                        listener->port = _libssh2_ntohu32(data + 1);
                        _libssh2_debug(session, LIBSSH2_TRACE_CONN,
                                       "Dynamic tcpip-forward port allocated: %d",
                                       listener->port);
                    }
                    else
                        listener->port = port;

                    listener->queue_size = 0;
                    listener->queue_maxsize = queue_maxsize;

                    /* append this to the parent's list of listeners */
                    _libssh2_list_add(&session->listeners, &listener->node);

                    if(bound_port) {
                        *bound_port = listener->port;
                    }
                }
            }

            LIBSSH2_FREE(session, data);
            session->fwdLstn_state = libssh2_NB_state_idle;
            return listener;
        }
        else if(data[0] == SSH_MSG_REQUEST_FAILURE) {
            LIBSSH2_FREE(session, data);
            _libssh2_error(session, LIBSSH2_ERROR_REQUEST_DENIED,
                           "Unable to complete request for forward-listen");
            session->fwdLstn_state = libssh2_NB_state_idle;
            return NULL;
        }
    }

    session->fwdLstn_state = libssh2_NB_state_idle;

    return NULL;
}