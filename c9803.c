send_ldap_result_ext(
    Slapi_PBlock *pb,
    int err,
    char *matched,
    char *text,
    int nentries,
    struct berval **urls,
    BerElement *ber)
{
    Slapi_Operation *operation;
    passwdPolicy *pwpolicy = NULL;
    Connection *conn = NULL;
    Slapi_DN *sdn = NULL;
    const char *dn = NULL;
    ber_tag_t tag;
    int flush_ber_element = 1;
    ber_tag_t bind_method = 0;
    int internal_op;
    int i, rc, logit = 0;
    char *pbtext;

    slapi_pblock_get(pb, SLAPI_BIND_METHOD, &bind_method);
    slapi_pblock_get(pb, SLAPI_OPERATION, &operation);
    slapi_pblock_get(pb, SLAPI_CONNECTION, &conn);

    if (text) {
        pbtext = text;
    } else {
        slapi_pblock_get(pb, SLAPI_PB_RESULT_TEXT, &pbtext);
    }

    if (operation == NULL) {
        slapi_log_err(SLAPI_LOG_ERR, "send_ldap_result_ext", "No operation found: slapi_search_internal_set_pb was incomplete (invalid 'base' ?)\n");
        return;
    }

    if (operation->o_status == SLAPI_OP_STATUS_RESULT_SENT) {
        return; /* result already sent */
    }

    if (ber != NULL) {
        flush_ber_element = 0;
    }

    if (err != LDAP_SUCCESS) {
        /* count the error for snmp */
        /* first check for security errors */
        if (err == LDAP_INVALID_CREDENTIALS || err == LDAP_INAPPROPRIATE_AUTH || err == LDAP_AUTH_METHOD_NOT_SUPPORTED || err == LDAP_STRONG_AUTH_NOT_SUPPORTED || err == LDAP_STRONG_AUTH_REQUIRED || err == LDAP_CONFIDENTIALITY_REQUIRED || err == LDAP_INSUFFICIENT_ACCESS || err == LDAP_AUTH_UNKNOWN) {
            slapi_counter_increment(g_get_global_snmp_vars()->ops_tbl.dsSecurityErrors);
        } else if (err != LDAP_REFERRAL && err != LDAP_OPT_REFERRALS && err != LDAP_PARTIAL_RESULTS) {
            /*madman man spec says not to count as normal errors
                --security errors
                --referrals
                -- partially seviced operations will not be conted as an error
                      */
            slapi_counter_increment(g_get_global_snmp_vars()->ops_tbl.dsErrors);
        }
    }

    slapi_log_err(SLAPI_LOG_TRACE, "send_ldap_result_ext", "=> %d:%s:%s\n", err,
                  matched ? matched : "", text ? text : "");

    switch (operation->o_tag) {
    case LBER_DEFAULT:
        tag = LBER_SEQUENCE;
        break;

    case LDAP_REQ_SEARCH:
        tag = LDAP_RES_SEARCH_RESULT;
        break;

    case LDAP_REQ_DELETE:
        tag = LDAP_RES_DELETE;
        break;

    case LDAP_REFERRAL:
        if (conn && conn->c_ldapversion > LDAP_VERSION2) {
            tag = LDAP_TAG_REFERRAL;
            break;
        }
    /* FALLTHROUGH */

    default:
        tag = operation->o_tag + 1;
        break;
    }

    internal_op = operation_is_flag_set(operation, OP_FLAG_INTERNAL);
    if ((conn == NULL) || (internal_op)) {
        if (operation->o_result_handler != NULL) {
            operation->o_result_handler(conn, operation, err,
                                        matched, text, nentries, urls);
            logit = 1;
        }
        goto log_and_return;
    }

    /* invalid password.  Update the password retry here */
    /* put this here for now.  It could be a send_result pre-op plugin. */
    if ((err == LDAP_INVALID_CREDENTIALS) && (bind_method != LDAP_AUTH_SASL)) {
        slapi_pblock_get(pb, SLAPI_TARGET_SDN, &sdn);
        dn = slapi_sdn_get_dn(sdn);
        pwpolicy = new_passwdPolicy(pb, dn);
        if (pwpolicy && (pwpolicy->pw_lockout == 1)) {
            if (update_pw_retry(pb) == LDAP_CONSTRAINT_VIOLATION && !pwpolicy->pw_is_legacy) {
                /*
                 * If we are not using the legacy pw policy behavior,
                 * convert the error 49 to 19 (constraint violation)
                 * and log a message
                 */
                err = LDAP_CONSTRAINT_VIOLATION;
                text = "Invalid credentials, you now have exceeded the password retry limit.";
            }
        }
    }

    if (ber == NULL) {
        if ((ber = der_alloc()) == NULL) {
            slapi_log_err(SLAPI_LOG_ERR, "send_ldap_result_ext", "ber_alloc failed\n");
            goto log_and_return;
        }
    }

    /* there is no admin limit exceeded in v2 - change to size limit XXX */
    if (err == LDAP_ADMINLIMIT_EXCEEDED &&
        conn->c_ldapversion < LDAP_VERSION3) {
        err = LDAP_SIZELIMIT_EXCEEDED;
    }

    if (conn->c_ldapversion < LDAP_VERSION3 || urls == NULL) {
        char *save, *buf = NULL;

        /*
         * if there are v2 referrals to send, construct
         * the v2 referral string.
         */
        if (urls != NULL) {
            int len;

            /* count the referral */
            slapi_counter_increment(g_get_global_snmp_vars()->ops_tbl.dsReferrals);

            /*
             * figure out how much space we need
             */
            len = 10; /* strlen("Referral:") + NULL */
            for (i = 0; urls[i] != NULL; i++) {
                len += urls[i]->bv_len + 1; /* newline + ref */
            }
            if (text != NULL) {
                len += strlen(text) + 1; /* text + newline */
            }
            /*
             * allocate buffer and fill it in with the error
             * message plus v2-style referrals.
             */
            buf = slapi_ch_malloc(len);
            *buf = '\0';
            if (text != NULL) {
                strcpy(buf, text);
                strcat(buf, "\n");
            }
            strcat(buf, "Referral:");
            for (i = 0; urls[i] != NULL; i++) {
                strcat(buf, "\n");
                strcat(buf, urls[i]->bv_val);
            }
            save = text;
            text = buf;
        }

        if ((conn->c_ldapversion < LDAP_VERSION3 &&
             err == LDAP_REFERRAL) ||
            urls != NULL) {
            err = LDAP_PARTIAL_RESULTS;
        }
        rc = ber_printf(ber, "{it{ess", operation->o_msgid, tag, err,
                        matched ? matched : "", pbtext ? pbtext : "");

        /*
         * if this is an LDAPv3 ExtendedResponse to an ExtendedRequest,
         * check to see if the optional responseName and response OCTET
         * STRING need to be appended.
         */
        if (rc != LBER_ERROR) {
            rc = check_and_send_extended_result(pb, tag, ber);
        }

        /*
         * if this is an LDAPv3 BindResponse, check to see if the
         * optional serverSaslCreds OCTET STRING is present and needs
         * to be appended.
         */
        if (rc != LBER_ERROR) {
            rc = check_and_send_SASL_response(pb, tag, ber, conn);
            /* XXXmcs: should we also check for a missing auth response control? */
        }

        if (rc != LBER_ERROR) {
            rc = ber_printf(ber, "}"); /* one more } to come */
        }

        if (buf != NULL) {
            text = save;
            slapi_ch_free((void **)&buf);
        }
    } else {
        /*
         * there are v3 referrals to add to the result
         */
        /* count the referral */
        if (!config_check_referral_mode())
            slapi_counter_increment(g_get_global_snmp_vars()->ops_tbl.dsReferrals);
        rc = ber_printf(ber, "{it{esst{s", operation->o_msgid, tag, err,
                        matched ? matched : "", text ? text : "", LDAP_TAG_REFERRAL,
                        urls[0]->bv_val);
        for (i = 1; urls[i] != NULL && rc != LBER_ERROR; i++) {
            rc = ber_printf(ber, "s", urls[i]->bv_val);
        }
        if (rc != LBER_ERROR) {
            rc = ber_printf(ber, "}"); /* two more } to come */
        }

        /*
         * if this is an LDAPv3 ExtendedResponse to an ExtendedRequest,
         * check to see if the optional responseName and response OCTET
         * STRING need to be appended.
         */
        if (rc != LBER_ERROR) {
            rc = check_and_send_extended_result(pb, tag, ber);
        }

        /*
         * if this is an LDAPv3 BindResponse, check to see if the
         * optional serverSaslCreds OCTET STRING is present and needs
         * to be appended.
         */
        if (rc != LBER_ERROR) {
            rc = check_and_send_SASL_response(pb, tag, ber, conn);
        }

        if (rc != LBER_ERROR) {
            rc = ber_printf(ber, "}"); /* one more } to come */
        }
    }
    if (err == LDAP_SUCCESS) {
        /*
         * Process the Read Entry Controls (if any)
         */
        if (process_read_entry_controls(pb, LDAP_CONTROL_PRE_READ_ENTRY)) {
            err = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
            goto log_and_return;
        }
        if (process_read_entry_controls(pb, LDAP_CONTROL_POST_READ_ENTRY)) {
            err = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
            goto log_and_return;
        }
    }
    if (operation->o_results.result_controls != NULL && conn->c_ldapversion >= LDAP_VERSION3 && write_controls(ber, operation->o_results.result_controls) != 0) {
        rc = (int)LBER_ERROR;
    }

    if (rc != LBER_ERROR) { /* end the LDAPMessage sequence */
        rc = ber_put_seq(ber);
    }

    if (rc == LBER_ERROR) {
        slapi_log_err(SLAPI_LOG_ERR, "send_ldap_result_ext", "ber_printf failed 1\n");
        if (flush_ber_element == 1) {
            /* we alloced the ber */
            ber_free(ber, 1 /* freebuf */);
        }
        goto log_and_return;
    }

    if (flush_ber_element) {
        /* write only one pdu at a time - wait til it's our turn */
        if (flush_ber(pb, conn, operation, ber, _LDAP_SEND_RESULT) == 0) {
            logit = 1;
        }
    }

log_and_return:
    operation->o_status = SLAPI_OP_STATUS_RESULT_SENT; /* in case this has not yet been set */

    if (logit && (operation_is_flag_set(operation, OP_FLAG_ACTION_LOG_ACCESS) ||
                  (internal_op && config_get_plugin_logging()))) {
        log_result(pb, operation, err, tag, nentries);
    }

    slapi_log_err(SLAPI_LOG_TRACE, "send_ldap_result_ext", "<= %d\n", err);
}