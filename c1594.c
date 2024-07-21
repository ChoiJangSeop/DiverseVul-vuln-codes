parse_be(const int is_emergency)
{
    char        lin[MAXBUF];
    BACKEND     *res;
    int         has_addr, has_port;
    struct hostent      *host;
    struct sockaddr_in  in;
    struct sockaddr_in6 in6;

    if((res = (BACKEND *)malloc(sizeof(BACKEND))) == NULL)
        conf_err("BackEnd config: out of memory - aborted");
    memset(res, 0, sizeof(BACKEND));
    res->be_type = 0;
    res->addr.ai_socktype = SOCK_STREAM;
    res->to = is_emergency? 120: be_to;
    res->conn_to = is_emergency? 120: be_connto;
    res->alive = 1;
    memset(&res->addr, 0, sizeof(res->addr));
    res->priority = 5;
    memset(&res->ha_addr, 0, sizeof(res->ha_addr));
    res->url = NULL;
    res->next = NULL;
    has_addr = has_port = 0;
    pthread_mutex_init(&res->mut, NULL);
    while(conf_fgets(lin, MAXBUF)) {
        if(strlen(lin) > 0 && lin[strlen(lin) - 1] == '\n')
            lin[strlen(lin) - 1] = '\0';
        if(!regexec(&Address, lin, 4, matches, 0)) {
            lin[matches[1].rm_eo] = '\0';
            if(get_host(lin + matches[1].rm_so, &res->addr)) {
                /* if we can't resolve it assume this is a UNIX domain socket */
                res->addr.ai_socktype = SOCK_STREAM;
                res->addr.ai_family = AF_UNIX;
                res->addr.ai_protocol = 0;
                if((res->addr.ai_addr = (struct sockaddr *)malloc(sizeof(struct sockaddr_un))) == NULL)
                    conf_err("out of memory");
                if((strlen(lin + matches[1].rm_so) + 1) > UNIX_PATH_MAX)
                    conf_err("UNIX path name too long");
                res->addr.ai_addrlen = strlen(lin + matches[1].rm_so) + 1;
                res->addr.ai_addr->sa_family = AF_UNIX;
                strcpy(res->addr.ai_addr->sa_data, lin + matches[1].rm_so);
                res->addr.ai_addrlen = sizeof( struct sockaddr_un );
            }
            has_addr = 1;
        } else if(!regexec(&Port, lin, 4, matches, 0)) {
            switch(res->addr.ai_family) {
            case AF_INET:
                memcpy(&in, res->addr.ai_addr, sizeof(in));
                in.sin_port = (in_port_t)htons(atoi(lin + matches[1].rm_so));
                memcpy(res->addr.ai_addr, &in, sizeof(in));
                break;
            case AF_INET6:
                memcpy(&in6, res->addr.ai_addr, sizeof(in6));
                in6.sin6_port = (in_port_t)htons(atoi(lin + matches[1].rm_so));
                memcpy(res->addr.ai_addr, &in6, sizeof(in6));
                break;
            default:
                conf_err("Port is supported only for INET/INET6 back-ends");
            }
            has_port = 1;
        } else if(!regexec(&Priority, lin, 4, matches, 0)) {
            if(is_emergency)
                conf_err("Priority is not supported for Emergency back-ends");
            res->priority = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&TimeOut, lin, 4, matches, 0)) {
            res->to = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&ConnTO, lin, 4, matches, 0)) {
            res->conn_to = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&HAport, lin, 4, matches, 0)) {
            if(is_emergency)
                conf_err("HAport is not supported for Emergency back-ends");
            res->ha_addr = res->addr;
            if((res->ha_addr.ai_addr = (struct sockaddr *)malloc(res->addr.ai_addrlen)) == NULL)
                conf_err("out of memory");
            memcpy(res->ha_addr.ai_addr, res->addr.ai_addr, res->addr.ai_addrlen);
            switch(res->addr.ai_family) {
            case AF_INET:
                memcpy(&in, res->ha_addr.ai_addr, sizeof(in));
                in.sin_port = (in_port_t)htons(atoi(lin + matches[1].rm_so));
                memcpy(res->ha_addr.ai_addr, &in, sizeof(in));
                break;
            case AF_INET6:
                memcpy(&in6, res->addr.ai_addr, sizeof(in6));
                in6.sin6_port = (in_port_t)htons(atoi(lin + matches[1].rm_so));
                memcpy(res->addr.ai_addr, &in6, sizeof(in6));
                break;
            default:
                conf_err("HAport is supported only for INET/INET6 back-ends");
            }
        } else if(!regexec(&HAportAddr, lin, 4, matches, 0)) {
            if(is_emergency)
                conf_err("HAportAddr is not supported for Emergency back-ends");
            lin[matches[1].rm_eo] = '\0';
            if(get_host(lin + matches[1].rm_so, &res->ha_addr)) {
                /* if we can't resolve it assume this is a UNIX domain socket */
                res->addr.ai_socktype = SOCK_STREAM;
                res->ha_addr.ai_family = AF_UNIX;
                res->ha_addr.ai_protocol = 0;
                if((res->ha_addr.ai_addr = (struct sockaddr *)strdup(lin + matches[1].rm_so)) == NULL)
                    conf_err("out of memory");
                res->addr.ai_addrlen = strlen(lin + matches[1].rm_so) + 1;
            } else switch(res->ha_addr.ai_family) {
            case AF_INET:
                memcpy(&in, res->ha_addr.ai_addr, sizeof(in));
                in.sin_port = (in_port_t)htons(atoi(lin + matches[2].rm_so));
                memcpy(res->ha_addr.ai_addr, &in, sizeof(in));
                break;
            case AF_INET6:
                memcpy(&in6, res->ha_addr.ai_addr, sizeof(in6));
                in6.sin6_port = (in_port_t)htons(atoi(lin + matches[2].rm_so));
                memcpy(res->ha_addr.ai_addr, &in6, sizeof(in6));
                break;
            default:
                conf_err("Unknown HA address type");
            }
        } else if(!regexec(&HTTPS, lin, 4, matches, 0)) {
            if((res->ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
                conf_err("SSL_CTX_new failed - aborted");
            SSL_CTX_set_app_data(res->ctx, res);
            SSL_CTX_set_verify(res->ctx, SSL_VERIFY_NONE, NULL);
            SSL_CTX_set_mode(res->ctx, SSL_MODE_AUTO_RETRY);
            SSL_CTX_set_options(res->ctx, SSL_OP_ALL);
            SSL_CTX_clear_options(res->ctx, SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);
            SSL_CTX_clear_options(res->ctx, SSL_OP_LEGACY_SERVER_CONNECT);
            sprintf(lin, "%d-Pound-%ld", getpid(), random());
            SSL_CTX_set_session_id_context(res->ctx, (unsigned char *)lin, strlen(lin));
            SSL_CTX_set_tmp_rsa_callback(res->ctx, RSA_tmp_callback);
            SSL_CTX_set_tmp_dh_callback(res->ctx, DH_tmp_callback);
        } else if(!regexec(&HTTPSCert, lin, 4, matches, 0)) {
            if((res->ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
                conf_err("SSL_CTX_new failed - aborted");
            SSL_CTX_set_app_data(res->ctx, res);
            lin[matches[1].rm_eo] = '\0';
            if(SSL_CTX_use_certificate_chain_file(res->ctx, lin + matches[1].rm_so) != 1)
                conf_err("SSL_CTX_use_certificate_chain_file failed - aborted");
            if(SSL_CTX_use_PrivateKey_file(res->ctx, lin + matches[1].rm_so, SSL_FILETYPE_PEM) != 1)
                conf_err("SSL_CTX_use_PrivateKey_file failed - aborted");
            if(SSL_CTX_check_private_key(res->ctx) != 1)
                conf_err("SSL_CTX_check_private_key failed - aborted");
            SSL_CTX_set_verify(res->ctx, SSL_VERIFY_NONE, NULL);
            SSL_CTX_set_mode(res->ctx, SSL_MODE_AUTO_RETRY);
            SSL_CTX_set_options(res->ctx, SSL_OP_ALL);
            SSL_CTX_clear_options(res->ctx, SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);
            SSL_CTX_clear_options(res->ctx, SSL_OP_LEGACY_SERVER_CONNECT);
            sprintf(lin, "%d-Pound-%ld", getpid(), random());
            SSL_CTX_set_session_id_context(res->ctx, (unsigned char *)lin, strlen(lin));
            SSL_CTX_set_tmp_rsa_callback(res->ctx, RSA_tmp_callback);
            SSL_CTX_set_tmp_dh_callback(res->ctx, DH_tmp_callback);
        } else if(!regexec(&Disabled, lin, 4, matches, 0)) {
            res->disabled = atoi(lin + matches[1].rm_so);
        } else if(!regexec(&End, lin, 4, matches, 0)) {
            if(!has_addr)
                conf_err("BackEnd missing Address - aborted");
            if((res->addr.ai_family == AF_INET || res->addr.ai_family == AF_INET6) && !has_port)
                conf_err("BackEnd missing Port - aborted");
            return res;
        } else {
            conf_err("unknown directive");
        }
    }

    conf_err("BackEnd premature EOF");
    return NULL;
}