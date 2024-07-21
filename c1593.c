parse_HTTPS(void)
{
    char        lin[MAXBUF];
    LISTENER    *res;
    SERVICE     *svc;
    MATCHER     *m;
    int         has_addr, has_port, has_other;
    long	ssl_op_enable, ssl_op_disable;
    struct hostent      *host;
    struct sockaddr_in  in;
    struct sockaddr_in6 in6;
    POUND_CTX   *pc;

    ssl_op_enable = SSL_OP_ALL;
    ssl_op_disable = SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION | SSL_OP_LEGACY_SERVER_CONNECT;

    if((res = (LISTENER *)malloc(sizeof(LISTENER))) == NULL)
        conf_err("ListenHTTPS config: out of memory - aborted");
    memset(res, 0, sizeof(LISTENER));

    res->to = clnt_to;
    res->rewr_loc = 1;
    res->err414 = "Request URI is too long";
    res->err500 = "An internal server error occurred. Please try again later.";
    res->err501 = "This method may not be used.";
    res->err503 = "The service is not available. Please try again later.";
    res->allow_client_reneg = 0;
    res->log_level = log_level;
    if(regcomp(&res->verb, xhttp[0], REG_ICASE | REG_NEWLINE | REG_EXTENDED))
        conf_err("xHTTP bad default pattern - aborted");
    has_addr = has_port = has_other = 0;
    while(conf_fgets(lin, MAXBUF)) {
        if(strlen(lin) > 0 && lin[strlen(lin) - 1] == '\n')
            lin[strlen(lin) - 1] = '\0';
        if(!regexec(&Address, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            if(get_host(lin + matches[1].rm_so, &res->addr))
                conf_err("Unknown Listener address");
            if(res->addr.ai_family != AF_INET && res->addr.ai_family != AF_INET6)
                conf_err("Unknown Listener address family");
            has_addr = 1;
        } else if(!regexec(&Port, lin, 4, matches, 0)) {
            if(res->addr.ai_family == AF_INET) {
                memcpy(&in, res->addr.ai_addr, sizeof(in));
                in.sin_port = (in_port_t)htons(atoi(lin + matches[1].rm_so));
                memcpy(res->addr.ai_addr, &in, sizeof(in));
            } else {
                memcpy(&in6, res->addr.ai_addr, sizeof(in6));
                in6.sin6_port = htons(atoi(lin + matches[1].rm_so));
                memcpy(res->addr.ai_addr, &in6, sizeof(in6));
            }
            has_port = 1;
        } else if(!regexec(&xHTTP, lin, 4, matches, 0)) {
            int n;

            n = atoi(lin + matches[1].rm_so);
            regfree(&res->verb);
            if(regcomp(&res->verb, xhttp[n], REG_ICASE | REG_NEWLINE | REG_EXTENDED))
                conf_err("xHTTP bad pattern - aborted");
        } else if(!regexec(&Client, lin, 4, matches, 0)) {
            res->to = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&CheckURL, lin, 4, matches, 0)) {
            if(res->has_pat)
                conf_err("CheckURL multiple pattern - aborted");
            lin[matches[1].rm_eo] = '\0';
            if(regcomp(&res->url_pat, lin + matches[1].rm_so, REG_NEWLINE | REG_EXTENDED | (ignore_case? REG_ICASE: 0)))
                conf_err("CheckURL bad pattern - aborted");
            res->has_pat = 1;
        } else if(!regexec(&Err414, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            res->err414 = file2str(lin + matches[1].rm_so);
        } else if(!regexec(&Err500, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            res->err500 = file2str(lin + matches[1].rm_so);
        } else if(!regexec(&Err501, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            res->err501 = file2str(lin + matches[1].rm_so);
        } else if(!regexec(&Err503, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            res->err503 = file2str(lin + matches[1].rm_so);
        } else if(!regexec(&MaxRequest, lin, 4, matches, 0)) {
            res->max_req = ATOL(lin + matches[1].rm_so);
        } else if(!regexec(&HeadRemove, lin, 4, matches, 0)) {
            if(res->head_off) {
                for(m = res->head_off; m->next; m = m->next)
                    ;
                if((m->next = (MATCHER *)malloc(sizeof(MATCHER))) == NULL)
                    conf_err("HeadRemove config: out of memory - aborted");
                m = m->next;
            } else {
                if((res->head_off = (MATCHER *)malloc(sizeof(MATCHER))) == NULL)
                    conf_err("HeadRemove config: out of memory - aborted");
                m = res->head_off;
            }
            memset(m, 0, sizeof(MATCHER));
            lin[matches[1].rm_eo] = '\0';
            if(regcomp(&m->pat, lin + matches[1].rm_so, REG_ICASE | REG_NEWLINE | REG_EXTENDED))
                conf_err("HeadRemove bad pattern - aborted");
        } else if(!regexec(&RewriteLocation, lin, 4, matches, 0)) {
            res->rewr_loc = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&RewriteDestination, lin, 4, matches, 0)) {
            res->rewr_dest = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&LogLevel, lin, 4, matches, 0)) {
            res->log_level = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&Cert, lin, 4, matches, 0)) {
#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
            /* we have support for SNI */
            FILE        *fcert;
            char        server_name[MAXBUF], *cp;
            X509        *x509;

            if(has_other)
                conf_err("Cert directives MUST precede other SSL-specific directives - aborted");
            if(res->ctx) {
                for(pc = res->ctx; pc->next; pc = pc->next)
                    ;
                if((pc->next = malloc(sizeof(POUND_CTX))) == NULL)
                    conf_err("ListenHTTPS new POUND_CTX: out of memory - aborted");
                pc = pc->next;
            } else {
                if((res->ctx = malloc(sizeof(POUND_CTX))) == NULL)
                    conf_err("ListenHTTPS new POUND_CTX: out of memory - aborted");
                pc = res->ctx;
            }
            if((pc->ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
                conf_err("SSL_CTX_new failed - aborted");
            pc->server_name = NULL;
            pc->next = NULL;
            lin[matches[1].rm_eo] = '\0';
            if(SSL_CTX_use_certificate_chain_file(pc->ctx, lin + matches[1].rm_so) != 1)
                conf_err("SSL_CTX_use_certificate_chain_file failed - aborted");
            if(SSL_CTX_use_PrivateKey_file(pc->ctx, lin + matches[1].rm_so, SSL_FILETYPE_PEM) != 1)
                conf_err("SSL_CTX_use_PrivateKey_file failed - aborted");
            if(SSL_CTX_check_private_key(pc->ctx) != 1)
                conf_err("SSL_CTX_check_private_key failed - aborted");
            if((fcert = fopen(lin + matches[1].rm_so, "r")) == NULL)
                conf_err("ListenHTTPS: could not open certificate file");
            if((x509 = PEM_read_X509(fcert, NULL, NULL, NULL)) == NULL)
                conf_err("ListenHTTPS: could not get certificate subject");
            fclose(fcert);
            memset(server_name, '\0', MAXBUF);
            X509_NAME_oneline(X509_get_subject_name(x509), server_name, MAXBUF - 1);
            X509_free(x509);
            if(!regexec(&CNName, server_name, 4, matches, 0)) {
                server_name[matches[1].rm_eo] = '\0';
                if((pc->server_name = strdup(server_name + matches[1].rm_so)) == NULL)
                    conf_err("ListenHTTPS: could not set certificate subject");
            } else
                conf_err("ListenHTTPS: could not get certificate CN");
#else
            /* no SNI support */
            if(has_other)
                conf_err("Cert directives MUST precede other SSL-specific directives - aborted");
            if(res->ctx)
                conf_err("ListenHTTPS: multiple certificates not supported - aborted");
            if((res->ctx = malloc(sizeof(POUND_CTX))) == NULL)
                conf_err("ListenHTTPS new POUND_CTX: out of memory - aborted");
            res->ctx->server_name = NULL;
            res->ctx->next = NULL;
            if((res->ctx->ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
                conf_err("SSL_CTX_new failed - aborted");
            lin[matches[1].rm_eo] = '\0';
            if(SSL_CTX_use_certificate_chain_file(res->ctx->ctx, lin + matches[1].rm_so) != 1)
                conf_err("SSL_CTX_use_certificate_chain_file failed - aborted");
            if(SSL_CTX_use_PrivateKey_file(res->ctx->ctx, lin + matches[1].rm_so, SSL_FILETYPE_PEM) != 1)
                conf_err("SSL_CTX_use_PrivateKey_file failed - aborted");
            if(SSL_CTX_check_private_key(res->ctx->ctx) != 1)
                conf_err("SSL_CTX_check_private_key failed - aborted");
#endif
        } else if(!regexec(&ClientCert, lin, 4, matches, 0)) {
            has_other = 1;
            if(res->ctx == NULL)
                conf_err("ClientCert may only be used after Cert - aborted");
            switch(res->clnt_check = atoi(lin + matches[1].rm_so)) {
            case 0:
                /* don't ask */
                for(pc = res->ctx; pc; pc = pc->next)
                    SSL_CTX_set_verify(pc->ctx, SSL_VERIFY_NONE, NULL);
                break;
            case 1:
                /* ask but OK if no client certificate */
                for(pc = res->ctx; pc; pc = pc->next) {
                    SSL_CTX_set_verify(pc->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);
                    SSL_CTX_set_verify_depth(pc->ctx, atoi(lin + matches[2].rm_so));
                }
                break;
            case 2:
                /* ask and fail if no client certificate */
                for(pc = res->ctx; pc; pc = pc->next) {
                    SSL_CTX_set_verify(pc->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
                    SSL_CTX_set_verify_depth(pc->ctx, atoi(lin + matches[2].rm_so));
                }
                break;
            case 3:
                /* ask but do not verify client certificate */
                for(pc = res->ctx; pc; pc = pc->next) {
                    SSL_CTX_set_verify(pc->ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_OK);
                    SSL_CTX_set_verify_depth(pc->ctx, atoi(lin + matches[2].rm_so));
                }
                break;
            }
        } else if(!regexec(&AddHeader, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            if((res->add_head = strdup(lin + matches[1].rm_so)) == NULL)
                conf_err("AddHeader config: out of memory - aborted");
        } else if(!regexec(&SSLAllowClientRenegotiation, lin, 4, matches, 0)) {
            res->allow_client_reneg = atoi(lin + matches[1].rm_so);
            if (res->allow_client_reneg == 2) {
                ssl_op_enable |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
                ssl_op_disable &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
            } else {
                ssl_op_disable |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
                ssl_op_enable &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
            }
        } else if(!regexec(&SSLHonorCipherOrder, lin, 4, matches, 0)) {
            if (atoi(lin + matches[1].rm_so)) {
                ssl_op_enable |= SSL_OP_CIPHER_SERVER_PREFERENCE;
                ssl_op_disable &= ~SSL_OP_CIPHER_SERVER_PREFERENCE;
            } else {
                ssl_op_disable |= SSL_OP_CIPHER_SERVER_PREFERENCE;
                ssl_op_enable &= ~SSL_OP_CIPHER_SERVER_PREFERENCE;
            }
        } else if(!regexec(&Ciphers, lin, 4, matches, 0)) {
            has_other = 1;
            if(res->ctx == NULL)
                conf_err("Ciphers may only be used after Cert - aborted");
            lin[matches[1].rm_eo] = '\0';
            for(pc = res->ctx; pc; pc = pc->next)
                SSL_CTX_set_cipher_list(pc->ctx, lin + matches[1].rm_so);
        } else if(!regexec(&CAlist, lin, 4, matches, 0)) {
            STACK_OF(X509_NAME) *cert_names;

            has_other = 1;
            if(res->ctx == NULL)
                conf_err("CAList may only be used after Cert - aborted");
            lin[matches[1].rm_eo] = '\0';
            if((cert_names = SSL_load_client_CA_file(lin + matches[1].rm_so)) == NULL)
                conf_err("SSL_load_client_CA_file failed - aborted");
            for(pc = res->ctx; pc; pc = pc->next)
                SSL_CTX_set_client_CA_list(pc->ctx, cert_names);
        } else if(!regexec(&VerifyList, lin, 4, matches, 0)) {
            has_other = 1;
            if(res->ctx == NULL)
                conf_err("VerifyList may only be used after Cert - aborted");
            lin[matches[1].rm_eo] = '\0';
            for(pc = res->ctx; pc; pc = pc->next)
                if(SSL_CTX_load_verify_locations(pc->ctx, lin + matches[1].rm_so, NULL) != 1)
                    conf_err("SSL_CTX_load_verify_locations failed - aborted");
        } else if(!regexec(&CRLlist, lin, 4, matches, 0)) {
#if HAVE_X509_STORE_SET_FLAGS
            X509_STORE *store;
            X509_LOOKUP *lookup;

            has_other = 1;
            if(res->ctx == NULL)
                conf_err("CRLlist may only be used after Cert - aborted");
            lin[matches[1].rm_eo] = '\0';
            for(pc = res->ctx; pc; pc = pc->next) {
                store = SSL_CTX_get_cert_store(pc->ctx);
                if((lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file())) == NULL)
                    conf_err("X509_STORE_add_lookup failed - aborted");
                if(X509_load_crl_file(lookup, lin + matches[1].rm_so, X509_FILETYPE_PEM) != 1)
                    conf_err("X509_load_crl_file failed - aborted");
                X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
            }
#else
            conf_err("your version of OpenSSL does not support CRL checking");
#endif
        } else if(!regexec(&NoHTTPS11, lin, 4, matches, 0)) {
            res->noHTTPS11 = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&Service, lin, 4, matches, 0)) {
            if(res->services == NULL)
                res->services = parse_service(NULL);
            else {
                for(svc = res->services; svc->next; svc = svc->next)
                    ;
                svc->next = parse_service(NULL);
            }
        } else if(!regexec(&ServiceName, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            if(res->services == NULL)
                res->services = parse_service(lin + matches[1].rm_so);
            else {
                for(svc = res->services; svc->next; svc = svc->next)
                    ;
                svc->next = parse_service(lin + matches[1].rm_so);
            }
        } else if(!regexec(&End, lin, 4, matches, 0)) {
            X509_STORE  *store;

            if(!has_addr || !has_port || res->ctx == NULL)
                conf_err("ListenHTTPS missing Address, Port or Certificate - aborted");
#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
            // Only set callback if we have more than one cert
            if(res->ctx->next)
                if(!SSL_CTX_set_tlsext_servername_callback(res->ctx->ctx, SNI_server_name)
                || !SSL_CTX_set_tlsext_servername_arg(res->ctx->ctx, res->ctx))
                    conf_err("ListenHTTPS: can't set SNI callback");
#endif
            for(pc = res->ctx; pc; pc = pc->next) {
                SSL_CTX_set_app_data(pc->ctx, res);
                SSL_CTX_set_mode(pc->ctx, SSL_MODE_AUTO_RETRY);
                SSL_CTX_set_options(pc->ctx, ssl_op_enable);
                SSL_CTX_clear_options(pc->ctx, ssl_op_disable);
                sprintf(lin, "%d-Pound-%ld", getpid(), random());
                SSL_CTX_set_session_id_context(pc->ctx, (unsigned char *)lin, strlen(lin));
                SSL_CTX_set_tmp_rsa_callback(pc->ctx, RSA_tmp_callback);
                SSL_CTX_set_tmp_dh_callback(pc->ctx, DH_tmp_callback);
                SSL_CTX_set_info_callback(pc->ctx, SSLINFO_callback);
            }
            return res;
        } else {
            conf_err("unknown directive");
        }
    }

    conf_err("ListenHTTPS premature EOF");
    return NULL;
}