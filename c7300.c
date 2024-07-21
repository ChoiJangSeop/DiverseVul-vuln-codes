session_startup(LIBSSH2_SESSION *session, libssh2_socket_t sock)
{
    int rc;

    if(session->startup_state == libssh2_NB_state_idle) {
        _libssh2_debug(session, LIBSSH2_TRACE_TRANS,
                       "session_startup for socket %d", sock);
        if(LIBSSH2_INVALID_SOCKET == sock) {
            /* Did we forget something? */
            return _libssh2_error(session, LIBSSH2_ERROR_BAD_SOCKET,
                                  "Bad socket provided");
        }
        session->socket_fd = sock;

        session->socket_prev_blockstate =
            !get_socket_nonblocking(session->socket_fd);

        if(session->socket_prev_blockstate) {
            /* If in blocking state change to non-blocking */
            rc = session_nonblock(session->socket_fd, 1);
            if(rc) {
                return _libssh2_error(session, rc,
                                      "Failed changing socket's "
                                      "blocking state to non-blocking");
            }
        }

        session->startup_state = libssh2_NB_state_created;
    }

    if(session->startup_state == libssh2_NB_state_created) {
        rc = banner_send(session);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
        else if(rc) {
            return _libssh2_error(session, rc,
                                  "Failed sending banner");
        }
        session->startup_state = libssh2_NB_state_sent;
        session->banner_TxRx_state = libssh2_NB_state_idle;
    }

    if(session->startup_state == libssh2_NB_state_sent) {
        do {
            rc = banner_receive(session);
            if(rc == LIBSSH2_ERROR_EAGAIN)
                return rc;
            else if(rc)
                return _libssh2_error(session, rc,
                                      "Failed getting banner");
        } while(strncmp("SSH-", (char *)session->remote.banner, 4));

        session->startup_state = libssh2_NB_state_sent1;
    }

    if(session->startup_state == libssh2_NB_state_sent1) {
        rc = _libssh2_kex_exchange(session, 0, &session->startup_key_state);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
        else if(rc)
            return _libssh2_error(session, rc,
                                  "Unable to exchange encryption keys");

        session->startup_state = libssh2_NB_state_sent2;
    }

    if(session->startup_state == libssh2_NB_state_sent2) {
        _libssh2_debug(session, LIBSSH2_TRACE_TRANS,
                       "Requesting userauth service");

        /* Request the userauth service */
        session->startup_service[0] = SSH_MSG_SERVICE_REQUEST;
        _libssh2_htonu32(session->startup_service + 1,
                         sizeof("ssh-userauth") - 1);
        memcpy(session->startup_service + 5, "ssh-userauth",
               sizeof("ssh-userauth") - 1);

        session->startup_state = libssh2_NB_state_sent3;
    }

    if(session->startup_state == libssh2_NB_state_sent3) {
        rc = _libssh2_transport_send(session, session->startup_service,
                                     sizeof("ssh-userauth") + 5 - 1,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return rc;
        else if(rc) {
            return _libssh2_error(session, rc,
                                  "Unable to ask for ssh-userauth service");
        }

        session->startup_state = libssh2_NB_state_sent4;
    }

    if(session->startup_state == libssh2_NB_state_sent4) {
        rc = _libssh2_packet_require(session, SSH_MSG_SERVICE_ACCEPT,
                                     &session->startup_data,
                                     &session->startup_data_len, 0, NULL, 0,
                                     &session->startup_req_state);
        if(rc)
            return rc;

        session->startup_service_length =
            _libssh2_ntohu32(session->startup_data + 1);

        if((session->startup_service_length != (sizeof("ssh-userauth") - 1))
            || strncmp("ssh-userauth", (char *) session->startup_data + 5,
                       session->startup_service_length)) {
            LIBSSH2_FREE(session, session->startup_data);
            session->startup_data = NULL;
            return _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                                  "Invalid response received from server");
        }
        LIBSSH2_FREE(session, session->startup_data);
        session->startup_data = NULL;

        session->startup_state = libssh2_NB_state_idle;

        return 0;
    }

    /* just for safety return some error */
    return LIBSSH2_ERROR_INVAL;
}