userauth_password(LIBSSH2_SESSION *session,
                  const char *username, unsigned int username_len,
                  const unsigned char *password, unsigned int password_len,
                  LIBSSH2_PASSWD_CHANGEREQ_FUNC((*passwd_change_cb)))
{
    unsigned char *s;
    static const unsigned char reply_codes[4] =
        { SSH_MSG_USERAUTH_SUCCESS, SSH_MSG_USERAUTH_FAILURE,
          SSH_MSG_USERAUTH_PASSWD_CHANGEREQ, 0
        };
    int rc;

    if(session->userauth_pswd_state == libssh2_NB_state_idle) {
        /* Zero the whole thing out */
        memset(&session->userauth_pswd_packet_requirev_state, 0,
               sizeof(session->userauth_pswd_packet_requirev_state));

        /*
         * 40 = packet_type(1) + username_len(4) + service_len(4) +
         * service(14)"ssh-connection" + method_len(4) + method(8)"password" +
         * chgpwdbool(1) + password_len(4) */
        session->userauth_pswd_data_len = username_len + 40;

        session->userauth_pswd_data0 =
            (unsigned char) ~SSH_MSG_USERAUTH_PASSWD_CHANGEREQ;

        /* TODO: remove this alloc with a fixed buffer in the session
           struct */
        s = session->userauth_pswd_data =
            LIBSSH2_ALLOC(session, session->userauth_pswd_data_len);
        if(!session->userauth_pswd_data) {
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Unable to allocate memory for "
                                  "userauth-password request");
        }

        *(s++) = SSH_MSG_USERAUTH_REQUEST;
        _libssh2_store_str(&s, username, username_len);
        _libssh2_store_str(&s, "ssh-connection", sizeof("ssh-connection") - 1);
        _libssh2_store_str(&s, "password", sizeof("password") - 1);
        *s++ = '\0';
        _libssh2_store_u32(&s, password_len);
        /* 'password' is sent separately */

        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Attempting to login using password authentication");

        session->userauth_pswd_state = libssh2_NB_state_created;
    }

    if(session->userauth_pswd_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_pswd_data,
                                     session->userauth_pswd_data_len,
                                     password, password_len);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                  "Would block writing password request");
        }

        /* now free the sent packet */
        LIBSSH2_FREE(session, session->userauth_pswd_data);
        session->userauth_pswd_data = NULL;

        if(rc) {
            session->userauth_pswd_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                                  "Unable to send userauth-password request");
        }

        session->userauth_pswd_state = libssh2_NB_state_sent;
    }

  password_response:

    if((session->userauth_pswd_state == libssh2_NB_state_sent)
        || (session->userauth_pswd_state == libssh2_NB_state_sent1)
        || (session->userauth_pswd_state == libssh2_NB_state_sent2)) {
        if(session->userauth_pswd_state == libssh2_NB_state_sent) {
            rc = _libssh2_packet_requirev(session, reply_codes,
                                          &session->userauth_pswd_data,
                                          &session->userauth_pswd_data_len,
                                          0, NULL, 0,
                                          &session->
                                          userauth_pswd_packet_requirev_state);

            if(rc) {
                if(rc != LIBSSH2_ERROR_EAGAIN)
                    session->userauth_pswd_state = libssh2_NB_state_idle;

                return _libssh2_error(session, rc,
                                      "Waiting for password response");
            }

            if(session->userauth_pswd_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
                _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                               "Password authentication successful");
                LIBSSH2_FREE(session, session->userauth_pswd_data);
                session->userauth_pswd_data = NULL;
                session->state |= LIBSSH2_STATE_AUTHENTICATED;
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return 0;
            }
            else if(session->userauth_pswd_data[0] == SSH_MSG_USERAUTH_FAILURE) {
                _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                               "Password authentication failed");
                LIBSSH2_FREE(session, session->userauth_pswd_data);
                session->userauth_pswd_data = NULL;
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                                      "Authentication failed "
                                      "(username/password)");
            }

            session->userauth_pswd_newpw = NULL;
            session->userauth_pswd_newpw_len = 0;

            session->userauth_pswd_state = libssh2_NB_state_sent1;
        }

        if((session->userauth_pswd_data[0] ==
             SSH_MSG_USERAUTH_PASSWD_CHANGEREQ)
            || (session->userauth_pswd_data0 ==
                SSH_MSG_USERAUTH_PASSWD_CHANGEREQ)) {
            session->userauth_pswd_data0 = SSH_MSG_USERAUTH_PASSWD_CHANGEREQ;

            if((session->userauth_pswd_state == libssh2_NB_state_sent1) ||
                (session->userauth_pswd_state == libssh2_NB_state_sent2)) {
                if(session->userauth_pswd_state == libssh2_NB_state_sent1) {
                    _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                                   "Password change required");
                    LIBSSH2_FREE(session, session->userauth_pswd_data);
                    session->userauth_pswd_data = NULL;
                }
                if(passwd_change_cb) {
                    if(session->userauth_pswd_state == libssh2_NB_state_sent1) {
                        passwd_change_cb(session,
                                         &session->userauth_pswd_newpw,
                                         &session->userauth_pswd_newpw_len,
                                         &session->abstract);
                        if(!session->userauth_pswd_newpw) {
                            return _libssh2_error(session,
                                                  LIBSSH2_ERROR_PASSWORD_EXPIRED,
                                                  "Password expired, and "
                                                  "callback failed");
                        }

                        /* basic data_len + newpw_len(4) */
                        session->userauth_pswd_data_len =
                            username_len + password_len + 44;

                        s = session->userauth_pswd_data =
                            LIBSSH2_ALLOC(session,
                                          session->userauth_pswd_data_len);
                        if(!session->userauth_pswd_data) {
                            LIBSSH2_FREE(session,
                                         session->userauth_pswd_newpw);
                            session->userauth_pswd_newpw = NULL;
                            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                                  "Unable to allocate memory "
                                                  "for userauth password "
                                                  "change request");
                        }

                        *(s++) = SSH_MSG_USERAUTH_REQUEST;
                        _libssh2_store_str(&s, username, username_len);
                        _libssh2_store_str(&s, "ssh-connection",
                                           sizeof("ssh-connection") - 1);
                        _libssh2_store_str(&s, "password",
                                           sizeof("password") - 1);
                        *s++ = 0x01;
                        _libssh2_store_str(&s, (char *)password, password_len);
                        _libssh2_store_u32(&s,
                                           session->userauth_pswd_newpw_len);
                        /* send session->userauth_pswd_newpw separately */

                        session->userauth_pswd_state = libssh2_NB_state_sent2;
                    }

                    if(session->userauth_pswd_state == libssh2_NB_state_sent2) {
                        rc = _libssh2_transport_send(session,
                                                     session->userauth_pswd_data,
                                                     session->userauth_pswd_data_len,
                                                     (unsigned char *)
                                                     session->userauth_pswd_newpw,
                                                     session->userauth_pswd_newpw_len);
                        if(rc == LIBSSH2_ERROR_EAGAIN) {
                            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                                  "Would block waiting");
                        }

                        /* free the allocated packets again */
                        LIBSSH2_FREE(session, session->userauth_pswd_data);
                        session->userauth_pswd_data = NULL;
                        LIBSSH2_FREE(session, session->userauth_pswd_newpw);
                        session->userauth_pswd_newpw = NULL;

                        if(rc) {
                            return _libssh2_error(session,
                                                  LIBSSH2_ERROR_SOCKET_SEND,
                                                  "Unable to send userauth "
                                                  "password-change request");
                        }

                        /*
                         * Ugliest use of goto ever.  Blame it on the
                         * askN => requirev migration.
                         */
                        session->userauth_pswd_state = libssh2_NB_state_sent;
                        goto password_response;
                    }
                }
            }
            else {
                session->userauth_pswd_state = libssh2_NB_state_idle;
                return _libssh2_error(session, LIBSSH2_ERROR_PASSWORD_EXPIRED,
                                      "Password Expired, and no callback "
                                      "specified");
            }
        }
    }

    /* FAILURE */
    LIBSSH2_FREE(session, session->userauth_pswd_data);
    session->userauth_pswd_data = NULL;
    session->userauth_pswd_state = libssh2_NB_state_idle;

    return _libssh2_error(session, LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                          "Authentication failed");
}