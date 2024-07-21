_libssh2_userauth_publickey(LIBSSH2_SESSION *session,
                            const char *username,
                            unsigned int username_len,
                            const unsigned char *pubkeydata,
                            unsigned long pubkeydata_len,
                            LIBSSH2_USERAUTH_PUBLICKEY_SIGN_FUNC((*sign_callback)),
                            void *abstract)
{
    unsigned char reply_codes[4] =
        { SSH_MSG_USERAUTH_SUCCESS, SSH_MSG_USERAUTH_FAILURE,
          SSH_MSG_USERAUTH_PK_OK, 0
        };
    int rc;
    unsigned char *s;

    if(session->userauth_pblc_state == libssh2_NB_state_idle) {

        /*
         * The call to _libssh2_ntohu32 later relies on pubkeydata having at
         * least 4 valid bytes containing the length of the method name.
         */
        if(pubkeydata_len < 4)
            return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                  "Invalid public key, too short");

        /* Zero the whole thing out */
        memset(&session->userauth_pblc_packet_requirev_state, 0,
               sizeof(session->userauth_pblc_packet_requirev_state));

        /*
         * As an optimisation, userauth_publickey_fromfile reuses a
         * previously allocated copy of the method name to avoid an extra
         * allocation/free.
         * For other uses, we allocate and populate it here.
         */
        if(!session->userauth_pblc_method) {
            session->userauth_pblc_method_len = _libssh2_ntohu32(pubkeydata);

            if(session->userauth_pblc_method_len > pubkeydata_len)
                /* the method length simply cannot be longer than the entire
                   passed in data, so we use this to detect crazy input
                   data */
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                      "Invalid public key");

            session->userauth_pblc_method =
                LIBSSH2_ALLOC(session, session->userauth_pblc_method_len);
            if(!session->userauth_pblc_method) {
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "Unable to allocate memory for public key "
                                      "data");
            }
            memcpy(session->userauth_pblc_method, pubkeydata + 4,
                   session->userauth_pblc_method_len);
        }
        /*
         * The length of the method name read from plaintext prefix in the
         * file must match length embedded in the key.
         * TODO: The data should match too but we don't check that. Should we?
         */
        else if(session->userauth_pblc_method_len !=
                 _libssh2_ntohu32(pubkeydata))
            return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                  "Invalid public key");

        /*
         * 45 = packet_type(1) + username_len(4) + servicename_len(4) +
         * service_name(14)"ssh-connection" + authmethod_len(4) +
         * authmethod(9)"publickey" + sig_included(1)'\0' + algmethod_len(4) +
         * publickey_len(4)
         */
        session->userauth_pblc_packet_len =
            username_len + session->userauth_pblc_method_len + pubkeydata_len +
            45;

        /*
         * Preallocate space for an overall length, method name again, and the
         * signature, which won't be any larger than the size of the
         * publickeydata itself.
         *
         * Note that the 'pubkeydata_len' extra bytes allocated here will not
         * be used in this first send, but will be used in the later one where
         * this same allocation is re-used.
         */
        s = session->userauth_pblc_packet =
            LIBSSH2_ALLOC(session,
                          session->userauth_pblc_packet_len + 4 +
                          (4 + session->userauth_pblc_method_len)
                          + (4 + pubkeydata_len));
        if(!session->userauth_pblc_packet) {
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Out of memory");
        }

        *s++ = SSH_MSG_USERAUTH_REQUEST;
        _libssh2_store_str(&s, username, username_len);
        _libssh2_store_str(&s, "ssh-connection", 14);
        _libssh2_store_str(&s, "publickey", 9);

        session->userauth_pblc_b = s;
        /* Not sending signature with *this* packet */
        *s++ = 0;

        _libssh2_store_str(&s, (const char *)session->userauth_pblc_method,
                           session->userauth_pblc_method_len);
        _libssh2_store_str(&s, (const char *)pubkeydata, pubkeydata_len);

        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Attempting publickey authentication");

        session->userauth_pblc_state = libssh2_NB_state_created;
    }

    if(session->userauth_pblc_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_pblc_packet,
                                     session->userauth_pblc_packet_len,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN)
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                                  "Unable to send userauth-publickey request");
        }

        session->userauth_pblc_state = libssh2_NB_state_sent;
    }

    if(session->userauth_pblc_state == libssh2_NB_state_sent) {
        rc = _libssh2_packet_requirev(session, reply_codes,
                                      &session->userauth_pblc_data,
                                      &session->userauth_pblc_data_len, 0,
                                      NULL, 0,
                                      &session->
                                      userauth_pblc_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                  "Waiting for USERAUTH response");
        }

        if(session->userauth_pblc_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
            _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                           "Pubkey authentication prematurely successful");
            /*
             * God help any SSH server that allows an UNVERIFIED
             * public key to validate the user
             */
            LIBSSH2_FREE(session, session->userauth_pblc_data);
            session->userauth_pblc_data = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            session->state |= LIBSSH2_STATE_AUTHENTICATED;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return 0;
        }

        if(session->userauth_pblc_data[0] == SSH_MSG_USERAUTH_FAILURE) {
            /* This public key is not allowed for this user on this server */
            LIBSSH2_FREE(session, session->userauth_pblc_data);
            session->userauth_pblc_data = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                                  "Username/PublicKey combination invalid");
        }

        /* Semi-Success! */
        LIBSSH2_FREE(session, session->userauth_pblc_data);
        session->userauth_pblc_data = NULL;

        *session->userauth_pblc_b = 0x01;
        session->userauth_pblc_state = libssh2_NB_state_sent1;
    }

    if(session->userauth_pblc_state == libssh2_NB_state_sent1) {
        unsigned char *buf;
        unsigned char *sig;
        size_t sig_len;

        s = buf = LIBSSH2_ALLOC(session, 4 + session->session_id_len
                                + session->userauth_pblc_packet_len);
        if(!buf) {
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Unable to allocate memory for "
                                  "userauth-publickey signed data");
        }

        _libssh2_store_str(&s, (const char *)session->session_id,
                           session->session_id_len);

        memcpy(s, session->userauth_pblc_packet,
               session->userauth_pblc_packet_len);
        s += session->userauth_pblc_packet_len;

        rc = sign_callback(session, &sig, &sig_len, buf, s - buf, abstract);
        LIBSSH2_FREE(session, buf);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_pblc_method);
            session->userauth_pblc_method = NULL;
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                  "Callback returned error");
        }

        /*
         * If this function was restarted, pubkeydata_len might still be 0
         * which will cause an unnecessary but harmless realloc here.
         */
        if(sig_len > pubkeydata_len) {
            unsigned char *newpacket;
            /* Should *NEVER* happen, but...well.. better safe than sorry */
            newpacket = LIBSSH2_REALLOC(session,
                                        session->userauth_pblc_packet,
                                        session->userauth_pblc_packet_len + 4 +
                                        (4 + session->userauth_pblc_method_len)
                                        + (4 + sig_len)); /* PK sigblob */
            if(!newpacket) {
                LIBSSH2_FREE(session, sig);
                LIBSSH2_FREE(session, session->userauth_pblc_packet);
                session->userauth_pblc_packet = NULL;
                LIBSSH2_FREE(session, session->userauth_pblc_method);
                session->userauth_pblc_method = NULL;
                session->userauth_pblc_state = libssh2_NB_state_idle;
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "Failed allocating additional space for "
                                      "userauth-publickey packet");
            }
            session->userauth_pblc_packet = newpacket;
        }

        s = session->userauth_pblc_packet + session->userauth_pblc_packet_len;
        session->userauth_pblc_b = NULL;

        _libssh2_store_u32(&s,
                           4 + session->userauth_pblc_method_len + 4 + sig_len);
        _libssh2_store_str(&s, (const char *)session->userauth_pblc_method,
                           session->userauth_pblc_method_len);

        LIBSSH2_FREE(session, session->userauth_pblc_method);
        session->userauth_pblc_method = NULL;

        _libssh2_store_str(&s, (const char *)sig, sig_len);
        LIBSSH2_FREE(session, sig);

        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Attempting publickey authentication -- phase 2");

        session->userauth_pblc_s = s;
        session->userauth_pblc_state = libssh2_NB_state_sent2;
    }

    if(session->userauth_pblc_state == libssh2_NB_state_sent2) {
        rc = _libssh2_transport_send(session, session->userauth_pblc_packet,
                                     session->userauth_pblc_s -
                                     session->userauth_pblc_packet,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_pblc_packet);
            session->userauth_pblc_packet = NULL;
            session->userauth_pblc_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                                  "Unable to send userauth-publickey request");
        }
        LIBSSH2_FREE(session, session->userauth_pblc_packet);
        session->userauth_pblc_packet = NULL;

        session->userauth_pblc_state = libssh2_NB_state_sent3;
    }

    /* PK_OK is no longer valid */
    reply_codes[2] = 0;

    rc = _libssh2_packet_requirev(session, reply_codes,
                                  &session->userauth_pblc_data,
                                  &session->userauth_pblc_data_len, 0, NULL, 0,
                                  &session->userauth_pblc_packet_requirev_state);
    if(rc == LIBSSH2_ERROR_EAGAIN) {
        return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                              "Would block requesting userauth list");
    }
    else if(rc) {
        session->userauth_pblc_state = libssh2_NB_state_idle;
        return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                              "Waiting for publickey USERAUTH response");
    }

    if(session->userauth_pblc_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Publickey authentication successful");
        /* We are us and we've proved it. */
        LIBSSH2_FREE(session, session->userauth_pblc_data);
        session->userauth_pblc_data = NULL;
        session->state |= LIBSSH2_STATE_AUTHENTICATED;
        session->userauth_pblc_state = libssh2_NB_state_idle;
        return 0;
    }

    /* This public key is not allowed for this user on this server */
    LIBSSH2_FREE(session, session->userauth_pblc_data);
    session->userauth_pblc_data = NULL;
    session->userauth_pblc_state = libssh2_NB_state_idle;
    return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                          "Invalid signature for supplied public key, or bad "
                          "username/public key combination");
}