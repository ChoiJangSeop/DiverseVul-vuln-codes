static char *userauth_list(LIBSSH2_SESSION *session, const char *username,
                           unsigned int username_len)
{
    static const unsigned char reply_codes[3] =
        { SSH_MSG_USERAUTH_SUCCESS, SSH_MSG_USERAUTH_FAILURE, 0 };
    /* packet_type(1) + username_len(4) + service_len(4) +
       service(14)"ssh-connection" + method_len(4) = 27 */
    unsigned long methods_len;
    unsigned char *s;
    int rc;

    if(session->userauth_list_state == libssh2_NB_state_idle) {
        /* Zero the whole thing out */
        memset(&session->userauth_list_packet_requirev_state, 0,
               sizeof(session->userauth_list_packet_requirev_state));

        session->userauth_list_data_len = username_len + 27;

        s = session->userauth_list_data =
            LIBSSH2_ALLOC(session, session->userauth_list_data_len);
        if(!session->userauth_list_data) {
            _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                           "Unable to allocate memory for userauth_list");
            return NULL;
        }

        *(s++) = SSH_MSG_USERAUTH_REQUEST;
        _libssh2_store_str(&s, username, username_len);
        _libssh2_store_str(&s, "ssh-connection", 14);
        _libssh2_store_u32(&s, 4); /* send "none" separately */

        session->userauth_list_state = libssh2_NB_state_created;
    }

    if(session->userauth_list_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_list_data,
                                     session->userauth_list_data_len,
                                     (unsigned char *)"none", 4);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                           "Would block requesting userauth list");
            return NULL;
        }
        /* now free the packet that was sent */
        LIBSSH2_FREE(session, session->userauth_list_data);
        session->userauth_list_data = NULL;

        if(rc) {
            _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                           "Unable to send userauth-none request");
            session->userauth_list_state = libssh2_NB_state_idle;
            return NULL;
        }

        session->userauth_list_state = libssh2_NB_state_sent;
    }

    if(session->userauth_list_state == libssh2_NB_state_sent) {
        rc = _libssh2_packet_requirev(session, reply_codes,
                                      &session->userauth_list_data,
                                      &session->userauth_list_data_len, 0,
                                      NULL, 0,
                                      &session->userauth_list_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                           "Would block requesting userauth list");
            return NULL;
        }
        else if(rc) {
            _libssh2_error(session, rc, "Failed getting response");
            session->userauth_list_state = libssh2_NB_state_idle;
            return NULL;
        }

        if(session->userauth_list_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
            /* Wow, who'dve thought... */
            _libssh2_error(session, LIBSSH2_ERROR_NONE, "No error");
            LIBSSH2_FREE(session, session->userauth_list_data);
            session->userauth_list_data = NULL;
            session->state |= LIBSSH2_STATE_AUTHENTICATED;
            session->userauth_list_state = libssh2_NB_state_idle;
            return NULL;
        }

        methods_len = _libssh2_ntohu32(session->userauth_list_data + 1);

        /* Do note that the memory areas overlap! */
        memmove(session->userauth_list_data, session->userauth_list_data + 5,
                methods_len);
        session->userauth_list_data[methods_len] = '\0';
        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Permitted auth methods: %s",
                       session->userauth_list_data);
    }

    session->userauth_list_state = libssh2_NB_state_idle;
    return (char *) session->userauth_list_data;
}