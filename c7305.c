userauth_hostbased_fromfile(LIBSSH2_SESSION *session,
                            const char *username, size_t username_len,
                            const char *publickey, const char *privatekey,
                            const char *passphrase, const char *hostname,
                            size_t hostname_len,
                            const char *local_username,
                            size_t local_username_len)
{
    int rc;

#if !LIBSSH2_RSA
    return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED,
                          "RSA is not supported by crypto backend");
#endif

    if(session->userauth_host_state == libssh2_NB_state_idle) {
        const LIBSSH2_HOSTKEY_METHOD *privkeyobj;
        unsigned char *pubkeydata, *sig = NULL;
        size_t pubkeydata_len = 0;
        size_t sig_len = 0;
        void *abstract;
        unsigned char buf[5];
        struct iovec datavec[4];

        /* Zero the whole thing out */
        memset(&session->userauth_host_packet_requirev_state, 0,
               sizeof(session->userauth_host_packet_requirev_state));

        if(publickey) {
            rc = file_read_publickey(session, &session->userauth_host_method,
                                     &session->userauth_host_method_len,
                                     &pubkeydata, &pubkeydata_len, publickey);
            if(rc)
                /* Note: file_read_publickey() calls _libssh2_error() */
                return rc;
        }
        else {
            /* Compute public key from private key. */
            rc = _libssh2_pub_priv_keyfile(session,
                                           &session->userauth_host_method,
                                           &session->userauth_host_method_len,
                                           &pubkeydata, &pubkeydata_len,
                                           privatekey, passphrase);
            if(rc)
                /* libssh2_pub_priv_keyfile calls _libssh2_error() */
                return rc;
        }

        /*
         * 52 = packet_type(1) + username_len(4) + servicename_len(4) +
         * service_name(14)"ssh-connection" + authmethod_len(4) +
         * authmethod(9)"hostbased" + method_len(4) + pubkeydata_len(4) +
         * hostname_len(4) + local_username_len(4)
         */
        session->userauth_host_packet_len =
            username_len + session->userauth_host_method_len + hostname_len +
            local_username_len + pubkeydata_len + 52;

        /*
         * Preallocate space for an overall length,  method name again,
         * and the signature, which won't be any larger than the size of
         * the publickeydata itself
         */
        session->userauth_host_s = session->userauth_host_packet =
            LIBSSH2_ALLOC(session,
                          session->userauth_host_packet_len + 4 +
                          (4 + session->userauth_host_method_len) +
                          (4 + pubkeydata_len));
        if(!session->userauth_host_packet) {
            LIBSSH2_FREE(session, session->userauth_host_method);
            session->userauth_host_method = NULL;
            LIBSSH2_FREE(session, pubkeydata);
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Out of memory");
        }

        *(session->userauth_host_s++) = SSH_MSG_USERAUTH_REQUEST;
        _libssh2_store_str(&session->userauth_host_s, username, username_len);
        _libssh2_store_str(&session->userauth_host_s, "ssh-connection", 14);
        _libssh2_store_str(&session->userauth_host_s, "hostbased", 9);
        _libssh2_store_str(&session->userauth_host_s,
                           (const char *)session->userauth_host_method,
                           session->userauth_host_method_len);
        _libssh2_store_str(&session->userauth_host_s, (const char *)pubkeydata,
                           pubkeydata_len);
        LIBSSH2_FREE(session, pubkeydata);
        _libssh2_store_str(&session->userauth_host_s, hostname, hostname_len);
        _libssh2_store_str(&session->userauth_host_s, local_username,
                           local_username_len);

        rc = file_read_privatekey(session, &privkeyobj, &abstract,
                                  session->userauth_host_method,
                                  session->userauth_host_method_len,
                                  privatekey, passphrase);
        if(rc) {
            /* Note: file_read_privatekey() calls _libssh2_error() */
            LIBSSH2_FREE(session, session->userauth_host_method);
            session->userauth_host_method = NULL;
            LIBSSH2_FREE(session, session->userauth_host_packet);
            session->userauth_host_packet = NULL;
            return rc;
        }

        _libssh2_htonu32(buf, session->session_id_len);
        libssh2_prepare_iovec(datavec, 4);
        datavec[0].iov_base = (void *)buf;
        datavec[0].iov_len = 4;
        datavec[1].iov_base = (void *)session->session_id;
        datavec[1].iov_len = session->session_id_len;
        datavec[2].iov_base = (void *)session->userauth_host_packet;
        datavec[2].iov_len = session->userauth_host_packet_len;

        if(privkeyobj && privkeyobj->signv &&
                          privkeyobj->signv(session, &sig, &sig_len, 3,
                                            datavec, &abstract)) {
            LIBSSH2_FREE(session, session->userauth_host_method);
            session->userauth_host_method = NULL;
            LIBSSH2_FREE(session, session->userauth_host_packet);
            session->userauth_host_packet = NULL;
            if(privkeyobj->dtor) {
                privkeyobj->dtor(session, &abstract);
            }
            return -1;
        }

        if(privkeyobj && privkeyobj->dtor) {
            privkeyobj->dtor(session, &abstract);
        }

        if(sig_len > pubkeydata_len) {
            unsigned char *newpacket;
            /* Should *NEVER* happen, but...well.. better safe than sorry */
            newpacket = LIBSSH2_REALLOC(session, session->userauth_host_packet,
                                        session->userauth_host_packet_len + 4 +
                                        (4 + session->userauth_host_method_len)
                                        + (4 + sig_len)); /* PK sigblob */
            if(!newpacket) {
                LIBSSH2_FREE(session, sig);
                LIBSSH2_FREE(session, session->userauth_host_packet);
                session->userauth_host_packet = NULL;
                LIBSSH2_FREE(session, session->userauth_host_method);
                session->userauth_host_method = NULL;
                return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                      "Failed allocating additional space for "
                                      "userauth-hostbased packet");
            }
            session->userauth_host_packet = newpacket;
        }

        session->userauth_host_s =
            session->userauth_host_packet + session->userauth_host_packet_len;

        _libssh2_store_u32(&session->userauth_host_s,
                           4 + session->userauth_host_method_len + 4 + sig_len);
        _libssh2_store_str(&session->userauth_host_s,
                           (const char *)session->userauth_host_method,
                           session->userauth_host_method_len);
        LIBSSH2_FREE(session, session->userauth_host_method);
        session->userauth_host_method = NULL;

        _libssh2_store_str(&session->userauth_host_s, (const char *)sig,
                           sig_len);
        LIBSSH2_FREE(session, sig);

        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Attempting hostbased authentication");

        session->userauth_host_state = libssh2_NB_state_created;
    }

    if(session->userauth_host_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_host_packet,
                                     session->userauth_host_s -
                                     session->userauth_host_packet,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_host_packet);
            session->userauth_host_packet = NULL;
            session->userauth_host_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                                  "Unable to send userauth-hostbased request");
        }
        LIBSSH2_FREE(session, session->userauth_host_packet);
        session->userauth_host_packet = NULL;

        session->userauth_host_state = libssh2_NB_state_sent;
    }

    if(session->userauth_host_state == libssh2_NB_state_sent) {
        static const unsigned char reply_codes[3] =
            { SSH_MSG_USERAUTH_SUCCESS, SSH_MSG_USERAUTH_FAILURE, 0 };
        size_t data_len;
        rc = _libssh2_packet_requirev(session, reply_codes,
                                      &session->userauth_host_data,
                                      &data_len, 0, NULL, 0,
                                      &session->
                                      userauth_host_packet_requirev_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }

        session->userauth_host_state = libssh2_NB_state_idle;
        if(rc) {
            return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                                  "Auth failed");
        }

        if(session->userauth_host_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
            _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                           "Hostbased authentication successful");
            /* We are us and we've proved it. */
            LIBSSH2_FREE(session, session->userauth_host_data);
            session->userauth_host_data = NULL;
            session->state |= LIBSSH2_STATE_AUTHENTICATED;
            return 0;
        }
    }

    /* This public key is not allowed for this user on this server */
    LIBSSH2_FREE(session, session->userauth_host_data);
    session->userauth_host_data = NULL;
    return _libssh2_error(session, LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED,
                          "Invalid signature for supplied public key, or bad "
                          "username/public key combination");
}