userauth_keyboard_interactive(LIBSSH2_SESSION * session,
                              const char *username,
                              unsigned int username_len,
                              LIBSSH2_USERAUTH_KBDINT_RESPONSE_FUNC((*response_callback)))
{
    unsigned char *s;
    int rc;

    static const unsigned char reply_codes[4] = {
        SSH_MSG_USERAUTH_SUCCESS,
        SSH_MSG_USERAUTH_FAILURE, SSH_MSG_USERAUTH_INFO_REQUEST, 0
    };
    unsigned int language_tag_len;
    unsigned int i;

    if(session->userauth_kybd_state == libssh2_NB_state_idle) {
        session->userauth_kybd_auth_name = NULL;
        session->userauth_kybd_auth_instruction = NULL;
        session->userauth_kybd_num_prompts = 0;
        session->userauth_kybd_auth_failure = 1;
        session->userauth_kybd_prompts = NULL;
        session->userauth_kybd_responses = NULL;

        /* Zero the whole thing out */
        memset(&session->userauth_kybd_packet_requirev_state, 0,
               sizeof(session->userauth_kybd_packet_requirev_state));

        session->userauth_kybd_packet_len =
            1                   /* byte    SSH_MSG_USERAUTH_REQUEST */
            + 4 + username_len  /* string  user name (ISO-10646 UTF-8, as
                                   defined in [RFC-3629]) */
            + 4 + 14            /* string  service name (US-ASCII) */
            + 4 + 20            /* string  "keyboard-interactive" (US-ASCII) */
            + 4 + 0             /* string  language tag (as defined in
                                   [RFC-3066]) */
            + 4 + 0             /* string  submethods (ISO-10646 UTF-8) */
            ;

        session->userauth_kybd_data = s =
            LIBSSH2_ALLOC(session, session->userauth_kybd_packet_len);
        if(!s) {
            return _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                  "Unable to allocate memory for "
                                  "keyboard-interactive authentication");
        }

        *s++ = SSH_MSG_USERAUTH_REQUEST;

        /* user name */
        _libssh2_store_str(&s, username, username_len);

        /* service name */
        _libssh2_store_str(&s, "ssh-connection", sizeof("ssh-connection") - 1);

        /* "keyboard-interactive" */
        _libssh2_store_str(&s, "keyboard-interactive",
                           sizeof("keyboard-interactive") - 1);
        /* language tag */
        _libssh2_store_u32(&s, 0);

        /* submethods */
        _libssh2_store_u32(&s, 0);

        _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                       "Attempting keyboard-interactive authentication");

        session->userauth_kybd_state = libssh2_NB_state_created;
    }

    if(session->userauth_kybd_state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, session->userauth_kybd_data,
                                     session->userauth_kybd_packet_len,
                                     NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN, "Would block");
        }
        else if(rc) {
            LIBSSH2_FREE(session, session->userauth_kybd_data);
            session->userauth_kybd_data = NULL;
            session->userauth_kybd_state = libssh2_NB_state_idle;
            return _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                                  "Unable to send keyboard-interactive request");
        }
        LIBSSH2_FREE(session, session->userauth_kybd_data);
        session->userauth_kybd_data = NULL;

        session->userauth_kybd_state = libssh2_NB_state_sent;
    }

    for(;;) {
        if(session->userauth_kybd_state == libssh2_NB_state_sent) {
            rc = _libssh2_packet_requirev(session, reply_codes,
                                          &session->userauth_kybd_data,
                                          &session->userauth_kybd_data_len,
                                          0, NULL, 0,
                                          &session->
                                          userauth_kybd_packet_requirev_state);
            if(rc == LIBSSH2_ERROR_EAGAIN) {
                return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                      "Would block");
            }
            else if(rc) {
                session->userauth_kybd_state = libssh2_NB_state_idle;
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                                      "Waiting for keyboard USERAUTH response");
            }

            if(session->userauth_kybd_data[0] == SSH_MSG_USERAUTH_SUCCESS) {
                _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                               "Keyboard-interactive authentication successful");
                LIBSSH2_FREE(session, session->userauth_kybd_data);
                session->userauth_kybd_data = NULL;
                session->state |= LIBSSH2_STATE_AUTHENTICATED;
                session->userauth_kybd_state = libssh2_NB_state_idle;
                return 0;
            }

            if(session->userauth_kybd_data[0] == SSH_MSG_USERAUTH_FAILURE) {
                _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                               "Keyboard-interactive authentication failed");
                LIBSSH2_FREE(session, session->userauth_kybd_data);
                session->userauth_kybd_data = NULL;
                session->userauth_kybd_state = libssh2_NB_state_idle;
                return _libssh2_error(session,
                                      LIBSSH2_ERROR_AUTHENTICATION_FAILED,
                                      "Authentication failed "
                                      "(keyboard-interactive)");
            }

            /* server requested PAM-like conversation */
            s = session->userauth_kybd_data + 1;

            /* string    name (ISO-10646 UTF-8) */
            session->userauth_kybd_auth_name_len = _libssh2_ntohu32(s);
            s += 4;
            if(session->userauth_kybd_auth_name_len) {
                session->userauth_kybd_auth_name =
                    LIBSSH2_ALLOC(session,
                                  session->userauth_kybd_auth_name_len);
                if(!session->userauth_kybd_auth_name) {
                    _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                   "Unable to allocate memory for "
                                   "keyboard-interactive 'name' "
                                   "request field");
                    goto cleanup;
                }
                memcpy(session->userauth_kybd_auth_name, s,
                       session->userauth_kybd_auth_name_len);
                s += session->userauth_kybd_auth_name_len;
            }

            /* string    instruction (ISO-10646 UTF-8) */
            session->userauth_kybd_auth_instruction_len = _libssh2_ntohu32(s);
            s += 4;
            if(session->userauth_kybd_auth_instruction_len) {
                session->userauth_kybd_auth_instruction =
                    LIBSSH2_ALLOC(session,
                                  session->userauth_kybd_auth_instruction_len);
                if(!session->userauth_kybd_auth_instruction) {
                    _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                   "Unable to allocate memory for "
                                   "keyboard-interactive 'instruction' "
                                   "request field");
                    goto cleanup;
                }
                memcpy(session->userauth_kybd_auth_instruction, s,
                       session->userauth_kybd_auth_instruction_len);
                s += session->userauth_kybd_auth_instruction_len;
            }

            /* string    language tag (as defined in [RFC-3066]) */
            language_tag_len = _libssh2_ntohu32(s);
            s += 4;

            /* ignoring this field as deprecated */
            s += language_tag_len;

            /* int       num-prompts */
            session->userauth_kybd_num_prompts = _libssh2_ntohu32(s);
            s += 4;

            if(session->userauth_kybd_num_prompts) {
                session->userauth_kybd_prompts =
                    LIBSSH2_CALLOC(session,
                                   sizeof(LIBSSH2_USERAUTH_KBDINT_PROMPT) *
                                   session->userauth_kybd_num_prompts);
                if(!session->userauth_kybd_prompts) {
                    _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                   "Unable to allocate memory for "
                                   "keyboard-interactive prompts array");
                    goto cleanup;
                }

                session->userauth_kybd_responses =
                    LIBSSH2_CALLOC(session,
                                   sizeof(LIBSSH2_USERAUTH_KBDINT_RESPONSE) *
                                   session->userauth_kybd_num_prompts);
                if(!session->userauth_kybd_responses) {
                    _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                   "Unable to allocate memory for "
                                   "keyboard-interactive responses array");
                    goto cleanup;
                }

                for(i = 0; i < session->userauth_kybd_num_prompts; i++) {
                    /* string    prompt[1] (ISO-10646 UTF-8) */
                    session->userauth_kybd_prompts[i].length =
                        _libssh2_ntohu32(s);
                    s += 4;
                    session->userauth_kybd_prompts[i].text =
                        LIBSSH2_CALLOC(session,
                                       session->userauth_kybd_prompts[i].length);
                    if(!session->userauth_kybd_prompts[i].text) {
                        _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                                       "Unable to allocate memory for "
                                       "keyboard-interactive prompt message");
                        goto cleanup;
                    }
                    memcpy(session->userauth_kybd_prompts[i].text, s,
                           session->userauth_kybd_prompts[i].length);
                    s += session->userauth_kybd_prompts[i].length;

                    /* boolean   echo[1] */
                    session->userauth_kybd_prompts[i].echo = *s++;
                }
            }

            response_callback(session->userauth_kybd_auth_name,
                              session->userauth_kybd_auth_name_len,
                              session->userauth_kybd_auth_instruction,
                              session->userauth_kybd_auth_instruction_len,
                              session->userauth_kybd_num_prompts,
                              session->userauth_kybd_prompts,
                              session->userauth_kybd_responses,
                              &session->abstract);

            _libssh2_debug(session, LIBSSH2_TRACE_AUTH,
                           "Keyboard-interactive response callback function"
                           " invoked");

            session->userauth_kybd_packet_len =
                1 /* byte      SSH_MSG_USERAUTH_INFO_RESPONSE */
                + 4             /* int       num-responses */
                ;

            for(i = 0; i < session->userauth_kybd_num_prompts; i++) {
                /* string    response[1] (ISO-10646 UTF-8) */
                session->userauth_kybd_packet_len +=
                    4 + session->userauth_kybd_responses[i].length;
            }

            /* A new userauth_kybd_data area is to be allocated, free the
               former one. */
            LIBSSH2_FREE(session, session->userauth_kybd_data);

            session->userauth_kybd_data = s =
                LIBSSH2_ALLOC(session, session->userauth_kybd_packet_len);
            if(!s) {
                _libssh2_error(session, LIBSSH2_ERROR_ALLOC,
                               "Unable to allocate memory for keyboard-"
                               "interactive response packet");
                goto cleanup;
            }

            *s = SSH_MSG_USERAUTH_INFO_RESPONSE;
            s++;
            _libssh2_store_u32(&s, session->userauth_kybd_num_prompts);

            for(i = 0; i < session->userauth_kybd_num_prompts; i++) {
                _libssh2_store_str(&s,
                                   session->userauth_kybd_responses[i].text,
                                   session->userauth_kybd_responses[i].length);
            }

            session->userauth_kybd_state = libssh2_NB_state_sent1;
        }

        if(session->userauth_kybd_state == libssh2_NB_state_sent1) {
            rc = _libssh2_transport_send(session, session->userauth_kybd_data,
                                         session->userauth_kybd_packet_len,
                                         NULL, 0);
            if(rc == LIBSSH2_ERROR_EAGAIN)
                return _libssh2_error(session, LIBSSH2_ERROR_EAGAIN,
                                      "Would block");
            if(rc) {
                _libssh2_error(session, LIBSSH2_ERROR_SOCKET_SEND,
                               "Unable to send userauth-keyboard-interactive"
                               " request");
                goto cleanup;
            }

            session->userauth_kybd_auth_failure = 0;
        }

      cleanup:
        /*
         * It's safe to clean all the data here, because unallocated pointers
         * are filled by zeroes
         */

        LIBSSH2_FREE(session, session->userauth_kybd_data);
        session->userauth_kybd_data = NULL;

        if(session->userauth_kybd_prompts) {
            for(i = 0; i < session->userauth_kybd_num_prompts; i++) {
                LIBSSH2_FREE(session, session->userauth_kybd_prompts[i].text);
                session->userauth_kybd_prompts[i].text = NULL;
            }
        }

        if(session->userauth_kybd_responses) {
            for(i = 0; i < session->userauth_kybd_num_prompts; i++) {
                LIBSSH2_FREE(session,
                             session->userauth_kybd_responses[i].text);
                session->userauth_kybd_responses[i].text = NULL;
            }
        }

        if(session->userauth_kybd_prompts) {
            LIBSSH2_FREE(session, session->userauth_kybd_prompts);
            session->userauth_kybd_prompts = NULL;
        }
        if(session->userauth_kybd_responses) {
            LIBSSH2_FREE(session, session->userauth_kybd_responses);
            session->userauth_kybd_responses = NULL;
        }
        if(session->userauth_kybd_auth_name) {
            LIBSSH2_FREE(session, session->userauth_kybd_auth_name);
            session->userauth_kybd_auth_name = NULL;
        }
        if(session->userauth_kybd_auth_instruction) {
            LIBSSH2_FREE(session, session->userauth_kybd_auth_instruction);
            session->userauth_kybd_auth_instruction = NULL;
        }

        if(session->userauth_kybd_auth_failure) {
            session->userauth_kybd_state = libssh2_NB_state_idle;
            return -1;
        }

        session->userauth_kybd_state = libssh2_NB_state_sent;
    }
}