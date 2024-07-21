 */
static int
kex_method_diffie_hellman_group_exchange_sha1_key_exchange
(LIBSSH2_SESSION * session, key_exchange_state_low_t * key_state)
{
    unsigned long p_len, g_len;
    int ret = 0;
    int rc;

    if(key_state->state == libssh2_NB_state_idle) {
        key_state->p = _libssh2_bn_init_from_bin();
        key_state->g = _libssh2_bn_init_from_bin();
        /* Ask for a P and G pair */
#ifdef LIBSSH2_DH_GEX_NEW
        key_state->request[0] = SSH_MSG_KEX_DH_GEX_REQUEST;
        _libssh2_htonu32(key_state->request + 1, LIBSSH2_DH_GEX_MINGROUP);
        _libssh2_htonu32(key_state->request + 5, LIBSSH2_DH_GEX_OPTGROUP);
        _libssh2_htonu32(key_state->request + 9, LIBSSH2_DH_GEX_MAXGROUP);
        key_state->request_len = 13;
        _libssh2_debug(session, LIBSSH2_TRACE_KEX,
                       "Initiating Diffie-Hellman Group-Exchange (New Method)");
#else
        key_state->request[0] = SSH_MSG_KEX_DH_GEX_REQUEST_OLD;
        _libssh2_htonu32(key_state->request + 1, LIBSSH2_DH_GEX_OPTGROUP);
        key_state->request_len = 5;
        _libssh2_debug(session, LIBSSH2_TRACE_KEX,
                       "Initiating Diffie-Hellman Group-Exchange (Old Method)");
#endif

        key_state->state = libssh2_NB_state_created;
    }

    if(key_state->state == libssh2_NB_state_created) {
        rc = _libssh2_transport_send(session, key_state->request,
                                     key_state->request_len, NULL, 0);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return rc;
        }
        else if(rc) {
            ret = _libssh2_error(session, rc,
                                 "Unable to send Group Exchange Request");
            goto dh_gex_clean_exit;
        }

        key_state->state = libssh2_NB_state_sent;
    }

    if(key_state->state == libssh2_NB_state_sent) {
        rc = _libssh2_packet_require(session, SSH_MSG_KEX_DH_GEX_GROUP,
                                     &key_state->data, &key_state->data_len,
                                     0, NULL, 0, &key_state->req_state);
        if(rc == LIBSSH2_ERROR_EAGAIN) {
            return rc;
        }
        else if(rc) {
            ret = _libssh2_error(session, rc,
                                 "Timeout waiting for GEX_GROUP reply");
            goto dh_gex_clean_exit;
        }

        key_state->state = libssh2_NB_state_sent1;
    }

    if(key_state->state == libssh2_NB_state_sent1) {
        unsigned char *s = key_state->data + 1;
        p_len = _libssh2_ntohu32(s);
        s += 4;
        _libssh2_bn_from_bin(key_state->p, p_len, s);
        s += p_len;

        g_len = _libssh2_ntohu32(s);
        s += 4;
        _libssh2_bn_from_bin(key_state->g, g_len, s);

        ret = diffie_hellman_sha1(session, key_state->g, key_state->p, p_len,
                                  SSH_MSG_KEX_DH_GEX_INIT,
                                  SSH_MSG_KEX_DH_GEX_REPLY,
                                  key_state->data + 1,
                                  key_state->data_len - 1,
                                  &key_state->exchange_state);
        if(ret == LIBSSH2_ERROR_EAGAIN) {
            return ret;
        }

        LIBSSH2_FREE(session, key_state->data);
    }

  dh_gex_clean_exit:
    key_state->state = libssh2_NB_state_idle;
    _libssh2_bn_free(key_state->g);
    key_state->g = NULL;
    _libssh2_bn_free(key_state->p);
    key_state->p = NULL;
