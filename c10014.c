NOEXPORT char *parse_service_option(CMD cmd, SERVICE_OPTIONS **section_ptr,
        char *opt, char *arg) {
    SERVICE_OPTIONS *section;
    int endpoints=0;
#ifndef USE_WIN32
    struct group *gr;
    struct passwd *pw;
#endif

    section=section_ptr ? *section_ptr : NULL;

    if(cmd==CMD_SET_DEFAULTS || cmd==CMD_SET_COPY) {
        section->ref=1;
    } else if(cmd==CMD_FREE) {
        if(section==&service_options || section==&new_service_options)
            s_log(LOG_DEBUG, "Deallocating section defaults");
        else
            s_log(LOG_DEBUG, "Deallocating section [%s]", section->servname);
    } else if(cmd==CMD_PRINT_DEFAULTS || cmd==CMD_PRINT_HELP) {
        s_log(LOG_NOTICE, " ");
        s_log(LOG_NOTICE, "Service-level options:");
    }

    /* accept */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->local_addr, 1);
        section->local_fd=NULL;
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->local_addr, 1);
        section->local_fd=NULL;
        name_list_dup(&section->local_addr.names,
            new_service_options.local_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->local_addr.names);
        str_free(section->local_addr.addr);
        str_free(section->local_fd);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "accept"))
            break;
        section->option.accept=1;
        name_list_append(&section->local_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->local_addr.names) {
            unsigned i;
            if(!addrlist_resolve(&section->local_addr))
                return "Cannot resolve accept target";
            section->local_fd=str_alloc_detached(section->local_addr.num*sizeof(SOCKET));
            for(i=0; i<section->local_addr.num; ++i)
                section->local_fd[i]=INVALID_SOCKET;
            ++endpoints;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port accept connections on specified host:port",
            "accept");
        break;
    }

    /* CApath */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#if 0
        section->ca_dir=(char *)X509_get_default_cert_dir();
#endif
        section->ca_dir=NULL;
        break;
    case CMD_SET_COPY:
        section->ca_dir=str_dup_detached(new_service_options.ca_dir);
        break;
    case CMD_FREE:
        str_free(section->ca_dir);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CApath"))
            break;
        str_free(section->ca_dir);
        if(arg[0]) /* not empty */
            section->ca_dir=str_dup_detached(arg);
        else
            section->ca_dir=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if 0
        s_log(LOG_NOTICE, "%-22s = %s", "CApath",
            section->ca_dir ? section->ca_dir : "(none)");
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CA certificate directory for 'verify' option",
            "CApath");
        break;
    }

    /* CAfile */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
#if 0
        section->ca_file=(char *)X509_get_default_certfile();
#endif
        section->ca_file=NULL;
        break;
    case CMD_SET_COPY:
        section->ca_file=str_dup_detached(new_service_options.ca_file);
        break;
    case CMD_FREE:
        str_free(section->ca_file);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CAfile"))
            break;
        str_free(section->ca_file);
        if(arg[0]) /* not empty */
            section->ca_file=str_dup_detached(arg);
        else
            section->ca_file=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if 0
        s_log(LOG_NOTICE, "%-22s = %s", "CAfile",
            section->ca_file ? section->ca_file : "(none)");
#endif
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CA certificate file for 'verify' option",
            "CAfile");
        break;
    }

    /* cert */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->cert=NULL;
        break;
    case CMD_SET_COPY:
        section->cert=str_dup_detached(new_service_options.cert);
        break;
    case CMD_FREE:
        str_free(section->cert);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "cert"))
            break;
        str_free(section->cert);
        section->cert=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
#ifndef OPENSSL_NO_PSK
        if(section->psk_keys)
            break;
#endif /* !defined(OPENSSL_NO_PSK) */
#ifndef OPENSSL_NO_ENGINE
        if(section->engine)
            break;
#endif /* !defined(OPENSSL_NO_ENGINE) */
        if(!section->option.client && !section->cert)
            return "TLS server needs a certificate";
        break;
    case CMD_PRINT_DEFAULTS:
        break; /* no default certificate */
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = certificate chain", "cert");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10002000L

    /* checkEmail */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_email=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_email,
            new_service_options.check_email);
        break;
    case CMD_FREE:
        name_list_free(section->check_email);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkEmail"))
            break;
        name_list_append(&section->check_email, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_email && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate email address",
            "checkEmail");
        break;
    }

    /* checkHost */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_host=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_host,
            new_service_options.check_host);
        break;
    case CMD_FREE:
        name_list_free(section->check_host);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkHost"))
            break;
        name_list_append(&section->check_host, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_host && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate host name pattern",
            "checkHost");
        break;
    }

    /* checkIP */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->check_ip=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->check_ip,
            new_service_options.check_ip);
        break;
    case CMD_FREE:
        name_list_free(section->check_ip);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "checkIP"))
            break;
        name_list_append(&section->check_ip, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->check_ip && !section->option.verify_chain && !section->option.verify_peer)
            return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = peer certificate IP address",
            "checkIP");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER>=0x10002000L */

    /* ciphers */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->cipher_list=NULL;
        break;
    case CMD_SET_COPY:
        section->cipher_list=str_dup_detached(new_service_options.cipher_list);
        break;
    case CMD_FREE:
        str_free(section->cipher_list);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ciphers"))
            break;
        str_free(section->cipher_list);
        section->cipher_list=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->cipher_list) {
            /* this is only executed for global options, because
             * section->cipher_list is no longer NULL in sections */
#ifdef USE_FIPS
            if(new_global_options.option.fips)
                section->cipher_list=str_dup_detached("FIPS");
            else
#endif /* USE_FIPS */
                section->cipher_list=str_dup_detached(stunnel_cipher_list);
        }
        break;
    case CMD_PRINT_DEFAULTS:
#ifdef USE_FIPS
        s_log(LOG_NOTICE, "%-22s = %s %s", "ciphers",
            "FIPS", "(with \"fips = yes\")");
        s_log(LOG_NOTICE, "%-22s = %s %s", "ciphers",
            stunnel_cipher_list, "(with \"fips = no\")");
#else
        s_log(LOG_NOTICE, "%-22s = %s", "ciphers", stunnel_cipher_list);
#endif /* USE_FIPS */
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = permitted ciphers for TLS 1.2 or older", "ciphers");
        break;
    }

#ifndef OPENSSL_NO_TLS1_3
    /* ciphersuites */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ciphersuites=NULL;
        break;
    case CMD_SET_COPY:
        section->ciphersuites=str_dup_detached(new_service_options.ciphersuites);
        break;
    case CMD_FREE:
        str_free(section->ciphersuites);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ciphersuites"))
            break;
        str_free(section->ciphersuites);
        section->ciphersuites=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ciphersuites) {
            /* this is only executed for global options, because
             * section->ciphersuites is no longer NULL in sections */
            section->ciphersuites=str_dup_detached(stunnel_ciphersuites);
        }
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s %s", "ciphersuites", stunnel_ciphersuites, "(with TLSv1.3)");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = permitted ciphersuites for TLS 1.3", "ciphersuites");
        break;
    }
#endif /* TLS 1.3 */

    /* client */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.client=0;
        break;
    case CMD_SET_COPY:
        section->option.client=new_service_options.option.client;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "client"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.client=1;
        else if(!strcasecmp(arg, "no"))
            section->option.client=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no client mode (remote service uses TLS)",
            "client");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10002000L

    /* config */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->config=NULL;
        break;
    case CMD_SET_COPY:
        name_list_dup(&section->config, new_service_options.config);
        break;
    case CMD_FREE:
        name_list_free(section->config);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "config"))
            break;
        name_list_append(&section->config, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = command[:parameter] to execute",
            "config");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER>=0x10002000L */

    /* connect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->connect_addr, 0);
        section->connect_session=NULL;
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->connect_addr, 0);
        section->connect_session=NULL;
        name_list_dup(&section->connect_addr.names,
            new_service_options.connect_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->connect_addr.names);
        str_free(section->connect_addr.addr);
        str_free(section->connect_session);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "connect"))
            break;
        name_list_append(&section->connect_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->connect_addr.names) {
            if(!section->option.delayed_lookup &&
                    !addrlist_resolve(&section->connect_addr)) {
                s_log(LOG_INFO,
                    "Cannot resolve connect target - delaying DNS lookup");
                section->connect_addr.num=0;
                section->redirect_addr.num=0;
                section->option.delayed_lookup=1;
            }
            if(section->option.client)
                section->connect_session=
                    str_alloc_detached(section->connect_addr.num*sizeof(SSL_SESSION *));
            ++endpoints;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port to connect",
            "connect");
        break;
    }

    /* CRLpath */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->crl_dir=NULL;
        break;
    case CMD_SET_COPY:
        section->crl_dir=str_dup_detached(new_service_options.crl_dir);
        break;
    case CMD_FREE:
        str_free(section->crl_dir);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CRLpath"))
            break;
        str_free(section->crl_dir);
        if(arg[0]) /* not empty */
            section->crl_dir=str_dup_detached(arg);
        else
            section->crl_dir=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CRL directory", "CRLpath");
        break;
    }

    /* CRLfile */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->crl_file=NULL;
        break;
    case CMD_SET_COPY:
        section->crl_file=str_dup_detached(new_service_options.crl_file);
        break;
    case CMD_FREE:
        str_free(section->crl_file);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "CRLfile"))
            break;
        str_free(section->crl_file);
        if(arg[0]) /* not empty */
            section->crl_file=str_dup_detached(arg);
        else
            section->crl_file=NULL;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = CRL file", "CRLfile");
        break;
    }

#ifndef OPENSSL_NO_ECDH

    /* curves */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->curves=str_dup_detached(DEFAULT_CURVES);
        break;
    case CMD_SET_COPY:
        section->curves=str_dup_detached(new_service_options.curves);
        break;
    case CMD_FREE:
        str_free(section->curves);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "curves") && strcasecmp(opt, "curve"))
            break;
        str_free(section->curves);
        section->curves=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "curves", DEFAULT_CURVES);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = ECDH curve names", "curves");
        break;
    }

#endif /* !defined(OPENSSL_NO_ECDH) */

    /* debug */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->log_level=LOG_NOTICE;
#if !defined (USE_WIN32) && !defined (__vms)
        new_global_options.log_facility=LOG_DAEMON;
#endif
        break;
    case CMD_SET_COPY:
        section->log_level=new_service_options.log_level;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "debug"))
            break;
        return parse_debug_level(arg, section);
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
#if !defined (USE_WIN32) && !defined (__vms)
        s_log(LOG_NOTICE, "%-22s = %s", "debug", "daemon.notice");
#else
        s_log(LOG_NOTICE, "%-22s = %s", "debug", "notice");
#endif
        break;
    case CMD_PRINT_HELP:
#if !defined (USE_WIN32) && !defined (__vms)
        s_log(LOG_NOTICE, "%-22s = [facility].level (e.g. daemon.info)", "debug");
#else
        s_log(LOG_NOTICE, "%-22s = level (e.g. info)", "debug");
#endif
        break;
    }

    /* delay */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.delayed_lookup=0;
        break;
    case CMD_SET_COPY:
        section->option.delayed_lookup=new_service_options.option.delayed_lookup;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "delay"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.delayed_lookup=1;
        else if(!strcasecmp(arg, "no"))
            section->option.delayed_lookup=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no delay DNS lookup for 'connect' option",
            "delay");
        break;
    }

#ifndef OPENSSL_NO_ENGINE

    /* engineId */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        section->engine=new_service_options.engine;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineId"))
            break;
        section->engine=engine_get_by_id(arg);
        if(!section->engine)
            return "Engine ID not found";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = ID of engine to read the key from",
            "engineId");
        break;
    }

    /* engineNum */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        section->engine=new_service_options.engine;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "engineNum"))
            break;
        {
            char *tmp_str;
            int tmp_int=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal engine number";
            section->engine=engine_get_by_num(tmp_int-1);
        }
        if(!section->engine)
            return "Illegal engine number";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = number of engine to read the key from",
            "engineNum");
        break;
    }

#endif /* !defined(OPENSSL_NO_ENGINE) */

    /* exec */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->exec_name=NULL;
        break;
    case CMD_SET_COPY:
        section->exec_name=str_dup_detached(new_service_options.exec_name);
        break;
    case CMD_FREE:
        str_free(section->exec_name);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "exec"))
            break;
        str_free(section->exec_name);
        section->exec_name=str_dup_detached(arg);
#ifdef USE_WIN32
        section->exec_args=str_dup_detached(arg);
#else
        if(!section->exec_args) {
            section->exec_args=str_alloc_detached(2*sizeof(char *));
            section->exec_args[0]=str_dup_detached(section->exec_name);
            section->exec_args[1]=NULL; /* null-terminate */
        }
#endif
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->exec_name)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = file execute local inetd-type program",
            "exec");
        break;
    }

    /* execArgs */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->exec_args=NULL;
        break;
    case CMD_SET_COPY:
#ifdef USE_WIN32
        section->exec_args=str_dup_detached(new_service_options.exec_args);
#else
        section->exec_args=arg_dup(new_service_options.exec_args);
#endif
        break;
    case CMD_FREE:
#ifdef USE_WIN32
        str_free(section->exec_args);
#else
        arg_free(section->exec_args);
#endif
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "execArgs"))
            break;
#ifdef USE_WIN32
        str_free(section->exec_args);
        section->exec_args=str_dup_detached(arg);
#else
        arg_free(section->exec_args);
        section->exec_args=arg_alloc(arg);
#endif
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = arguments for 'exec' (including $0)",
            "execArgs");
        break;
    }

    /* failover */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->failover=FAILOVER_PRIO;
        section->rr=0;
        break;
    case CMD_SET_COPY:
        section->failover=new_service_options.failover;
        section->rr=new_service_options.rr;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "failover"))
            break;
        if(!strcasecmp(arg, "rr"))
            section->failover=FAILOVER_RR;
        else if(!strcasecmp(arg, "prio"))
            section->failover=FAILOVER_PRIO;
        else
            return "The argument needs to be either 'rr' or 'prio'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->option.delayed_lookup)
            section->failover=FAILOVER_PRIO;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = rr|prio failover strategy",
            "failover");
        break;
    }

    /* ident */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->username=NULL;
        break;
    case CMD_SET_COPY:
        section->username=str_dup_detached(new_service_options.username);
        break;
    case CMD_FREE:
        str_free(section->username);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ident"))
            break;
        str_free(section->username);
        section->username=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for IDENT (RFC 1413) checking", "ident");
        break;
    }

    /* include */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "include"))
            break;
        return include_config(arg, section_ptr);
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = directory with configuration file snippets",
            "include");
        break;
    }

    /* key */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->key=NULL;
        break;
    case CMD_SET_COPY:
        section->key=str_dup_detached(new_service_options.key);
        break;
    case CMD_FREE:
        str_free(section->key);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "key"))
            break;
        str_free(section->key);
        section->key=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->cert && !section->key)
            section->key=str_dup_detached(section->cert);
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = certificate private key", "key");
        break;
    }

    /* libwrap */
#ifdef USE_LIBWRAP
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.libwrap=0; /* disable libwrap by default */
        break;
    case CMD_SET_COPY:
        section->option.libwrap=new_service_options.option.libwrap;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "libwrap"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.libwrap=1;
        else if(!strcasecmp(arg, "no"))
            section->option.libwrap=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no use /etc/hosts.allow and /etc/hosts.deny",
            "libwrap");
        break;
    }
#endif /* USE_LIBWRAP */

    /* local */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.local=0;
        break;
    case CMD_SET_COPY:
        section->option.local=new_service_options.option.local;
        memcpy(&section->source_addr, &new_service_options.source_addr,
            sizeof(SOCKADDR_UNION));
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "local"))
            break;
        if(!hostport2addr(&section->source_addr, arg, "0", 1))
            return "Failed to resolve local address";
        section->option.local=1;
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = IP address to be used as source for remote"
            " connections", "local");
        break;
    }

    /* logId */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->log_id=LOG_ID_SEQUENTIAL;
        break;
    case CMD_SET_COPY:
        section->log_id=new_service_options.log_id;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "logId"))
            break;
        if(!strcasecmp(arg, "sequential"))
            section->log_id=LOG_ID_SEQUENTIAL;
        else if(!strcasecmp(arg, "unique"))
            section->log_id=LOG_ID_UNIQUE;
        else if(!strcasecmp(arg, "thread"))
            section->log_id=LOG_ID_THREAD;
        else if(!strcasecmp(arg, "process"))
            section->log_id=LOG_ID_PROCESS;
        else
            return "Invalid connection identifier type";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "logId", "sequential");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = connection identifier type",
            "logId");
        break;
    }

#ifndef OPENSSL_NO_OCSP

    /* OCSP */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ocsp_url=NULL;
        break;
    case CMD_SET_COPY:
        section->ocsp_url=str_dup_detached(new_service_options.ocsp_url);
        break;
    case CMD_FREE:
        str_free(section->ocsp_url);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ocsp"))
            break;
        str_free(section->ocsp_url);
        section->ocsp_url=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = OCSP responder URL", "OCSP");
        break;
    }

    /* OCSPaia */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.aia=0; /* disable AIA by default */
        break;
    case CMD_SET_COPY:
        section->option.aia=new_service_options.option.aia;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPaia"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.aia=1;
        else if(!strcasecmp(arg, "no"))
            section->option.aia=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no check the AIA responders from certificates",
            "OCSPaia");
        break;
    }

    /* OCSPflag */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ocsp_flags=0;
        break;
    case CMD_SET_COPY:
        section->ocsp_flags=new_service_options.ocsp_flags;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPflag"))
            break;
        {
            unsigned long tmp_ulong=parse_ocsp_flag(arg);
            if(!tmp_ulong)
                return "Illegal OCSP flag";
            section->ocsp_flags|=tmp_ulong;
        }
        return NULL;
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = OCSP responder flags", "OCSPflag");
        break;
    }

    /* OCSPnonce */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.nonce=0; /* disable OCSP nonce by default */
        break;
    case CMD_SET_COPY:
        section->option.nonce=new_service_options.option.nonce;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "OCSPnonce"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.nonce=1;
        else if(!strcasecmp(arg, "no"))
            section->option.nonce=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = yes|no send and verify the OCSP nonce extension",
            "OCSPnonce");
        break;
    }

#endif /* !defined(OPENSSL_NO_OCSP) */

    /* options */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ssl_options_set=0;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        section->ssl_options_clear=0;
#endif /* OpenSSL 0.9.8m or later */
        break;
    case CMD_SET_COPY:
        section->ssl_options_set=new_service_options.ssl_options_set;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        section->ssl_options_clear=new_service_options.ssl_options_clear;
#endif /* OpenSSL 0.9.8m or later */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "options"))
            break;
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
        if(*arg=='-') {
            long unsigned tmp=parse_ssl_option(arg+1);
            if(tmp==INVALID_SSL_OPTION)
                return "Illegal TLS option";
            section->ssl_options_clear|=tmp;
            return NULL; /* OK */
        }
#endif /* OpenSSL 0.9.8m or later */
        {
            long unsigned tmp=parse_ssl_option(arg);
            if(tmp==INVALID_SSL_OPTION)
                return "Illegal TLS option";
            section->ssl_options_set|=tmp;
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %s", "options", "NO_SSLv2");
        s_log(LOG_NOTICE, "%-22s = %s", "options", "NO_SSLv3");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = TLS option to set/reset", "options");
        break;
    }

    /* protocol */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol=str_dup_detached(new_service_options.protocol);
        break;
    case CMD_FREE:
        str_free(section->protocol);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocol"))
            break;
        str_free(section->protocol);
        section->protocol=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        /* PROTOCOL_CHECK also initializes:
           section->option.connect_before_ssl
           section->option.protocol_endpoint */
        {
            char *tmp_str=protocol(NULL, section, PROTOCOL_CHECK);
            if(tmp_str)
                return tmp_str;
        }
        endpoints+=section->option.protocol_endpoint;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = protocol to negotiate before TLS initialization",
            "protocol");
        s_log(LOG_NOTICE, "%25scurrently supported: cifs, connect, imap,", "");
        s_log(LOG_NOTICE, "%25s    nntp, pgsql, pop3, proxy, smtp, socks", "");
        break;
    }

    /* protocolAuthentication */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_authentication=str_dup_detached("basic");
        break;
    case CMD_SET_COPY:
        section->protocol_authentication=
            str_dup_detached(new_service_options.protocol_authentication);
        break;
    case CMD_FREE:
        str_free(section->protocol_authentication);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolAuthentication"))
            break;
        str_free(section->protocol_authentication);
        section->protocol_authentication=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = authentication type for protocol negotiations",
            "protocolAuthentication");
        break;
    }

    /* protocolDomain */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_domain=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_domain=
            str_dup_detached(new_service_options.protocol_domain);
        break;
    case CMD_FREE:
        str_free(section->protocol_domain);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolDomain"))
            break;
        str_free(section->protocol_domain);
        section->protocol_domain=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = domain for protocol negotiations",
            "protocolDomain");
        break;
    }

    /* protocolHost */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_host=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_host=
            str_dup_detached(new_service_options.protocol_host);
        break;
    case CMD_FREE:
        str_free(section->protocol_host);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolHost"))
            break;
        str_free(section->protocol_host);
        section->protocol_host=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = host:port for protocol negotiations",
            "protocolHost");
        break;
    }

    /* protocolPassword */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_password=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_password=
            str_dup_detached(new_service_options.protocol_password);
        break;
    case CMD_FREE:
        str_free(section->protocol_password);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolPassword"))
            break;
        str_free(section->protocol_password);
        section->protocol_password=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = password for protocol negotiations",
            "protocolPassword");
        break;
    }

    /* protocolUsername */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->protocol_username=NULL;
        break;
    case CMD_SET_COPY:
        section->protocol_username=
            str_dup_detached(new_service_options.protocol_username);
        break;
    case CMD_FREE:
        str_free(section->protocol_username);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "protocolUsername"))
            break;
        str_free(section->protocol_username);
        section->protocol_username=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for protocol negotiations",
            "protocolUsername");
        break;
    }

#ifndef OPENSSL_NO_PSK

    /* PSKidentity */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->psk_identity=NULL;
        section->psk_selected=NULL;
        section->psk_sorted.val=NULL;
        section->psk_sorted.num=0;
        break;
    case CMD_SET_COPY:
        section->psk_identity=
            str_dup_detached(new_service_options.psk_identity);
        break;
    case CMD_FREE:
        str_free(section->psk_identity);
        str_free(section->psk_sorted.val);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "PSKidentity"))
            break;
        str_free(section->psk_identity);
        section->psk_identity=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->psk_keys) /* PSK not configured */
            break;
        psk_sort(&section->psk_sorted, section->psk_keys);
        if(section->option.client) {
            if(section->psk_identity) {
                section->psk_selected=
                    psk_find(&section->psk_sorted, section->psk_identity);
                if(!section->psk_selected)
                    return "No key found for the specified PSK identity";
            } else { /* take the first specified identity as default */
                section->psk_selected=section->psk_keys;
            }
        } else {
            if(section->psk_identity)
                s_log(LOG_NOTICE,
                    "PSK identity is ignored in the server mode");
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = identity for PSK authentication",
            "PSKidentity");
        break;
    }

    /* PSKsecrets */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->psk_keys=NULL;
        break;
    case CMD_SET_COPY:
        section->psk_keys=psk_dup(new_service_options.psk_keys);
        break;
    case CMD_FREE:
        psk_free(section->psk_keys);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "PSKsecrets"))
            break;
        section->psk_keys=psk_read(arg);
        if(!section->psk_keys)
            return "Failed to read PSK secrets";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = secrets for PSK authentication",
            "PSKsecrets");
        break;
    }

#endif /* !defined(OPENSSL_NO_PSK) */

    /* pty */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.pty=0;
        break;
    case CMD_SET_COPY:
        section->option.pty=new_service_options.option.pty;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "pty"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.pty=1;
        else if(!strcasecmp(arg, "no"))
            section->option.pty=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no allocate pseudo terminal for 'exec' option",
            "pty");
        break;
    }
#endif

    /* redirect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        addrlist_clear(&section->redirect_addr, 0);
        break;
    case CMD_SET_COPY:
        addrlist_clear(&section->redirect_addr, 0);
        name_list_dup(&section->redirect_addr.names,
            new_service_options.redirect_addr.names);
        break;
    case CMD_FREE:
        name_list_free(section->redirect_addr.names);
        str_free(section->redirect_addr.addr);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "redirect"))
            break;
        name_list_append(&section->redirect_addr.names, arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->redirect_addr.names) {
            if(section->option.client)
                return "\"redirect\" is unsupported in client sections";
            if(section->option.connect_before_ssl)
                return "\"redirect\" is incompatible with the specified protocol negotiation";
            if(!section->option.delayed_lookup &&
                    !addrlist_resolve(&section->redirect_addr)) {
                s_log(LOG_INFO,
                    "Cannot resolve redirect target - delaying DNS lookup");
                section->connect_addr.num=0;
                section->redirect_addr.num=0;
                section->option.delayed_lookup=1;
            }
            if(!section->option.verify_chain && !section->option.verify_peer)
                return "Either \"verifyChain\" or \"verifyPeer\" has to be enabled for \"redirect\" to work";
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = [host:]port to redirect on authentication failures",
            "redirect");
        break;
    }

    /* renegotiation */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.renegotiation=1;
        break;
    case CMD_SET_COPY:
        section->option.renegotiation=new_service_options.option.renegotiation;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "renegotiation"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.renegotiation=1;
        else if(!strcasecmp(arg, "no"))
            section->option.renegotiation=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no support renegotiation",
              "renegotiation");
        break;
    }

    /* requireCert */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.require_cert=0;
        break;
    case CMD_SET_COPY:
        section->option.require_cert=new_service_options.option.require_cert;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "requireCert"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.require_cert=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no require client certificate",
            "requireCert");
        break;
    }

    /* reset */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.reset=1; /* enabled by default */
        break;
    case CMD_SET_COPY:
        section->option.reset=new_service_options.option.reset;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "reset"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.reset=1;
        else if(!strcasecmp(arg, "no"))
            section->option.reset=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no send TCP RST on error",
            "reset");
        break;
    }

    /* retry */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.retry=0;
        break;
    case CMD_SET_COPY:
        section->option.retry=new_service_options.option.retry;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "retry"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.retry=1;
        else if(!strcasecmp(arg, "no"))
            section->option.retry=0;
        else
            return "The argument needs to be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no retry connect+exec section",
            "retry");
        break;
    }

#ifndef USE_WIN32
    /* service */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->servname=str_dup_detached("stunnel");
        break;
    case CMD_SET_COPY:
        /* servname is *not* copied from the global section */
        break;
    case CMD_FREE:
        /* deallocation is performed at the end CMD_FREE */
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "service"))
            break;
        str_free(section->servname);
        section->servname=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = service name", "service");
        break;
    }
#endif

#ifndef USE_WIN32
    /* setgid */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->gid=0;
        break;
    case CMD_SET_COPY:
        section->gid=new_service_options.gid;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "setgid"))
            break;
        gr=getgrnam(arg);
        if(gr) {
            section->gid=gr->gr_gid;
            return NULL; /* OK */
        }
        {
            char *tmp_str;
            section->gid=(gid_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal GID";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = groupname for setgid()", "setgid");
        break;
    }
#endif

#ifndef USE_WIN32
    /* setuid */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->uid=0;
        break;
    case CMD_SET_COPY:
        section->uid=new_service_options.uid;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "setuid"))
            break;
        pw=getpwnam(arg);
        if(pw) {
            section->uid=pw->pw_uid;
            return NULL; /* OK */
        }
        {
            char *tmp_str;
            section->uid=(uid_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal UID";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = username for setuid()", "setuid");
        break;
    }
#endif

    /* sessionCacheSize */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->session_size=1000L;
        break;
    case CMD_SET_COPY:
        section->session_size=new_service_options.session_size;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessionCacheSize"))
            break;
        {
            char *tmp_str;
            section->session_size=strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal session cache size";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %ld", "sessionCacheSize", 1000L);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = session cache size", "sessionCacheSize");
        break;
    }

    /* sessionCacheTimeout */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->session_timeout=300L;
        break;
    case CMD_SET_COPY:
        section->session_timeout=new_service_options.session_timeout;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessionCacheTimeout") && strcasecmp(opt, "session"))
            break;
        {
            char *tmp_str;
            section->session_timeout=strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal session cache timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %ld seconds", "sessionCacheTimeout", 300L);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = session cache timeout (in seconds)",
            "sessionCacheTimeout");
        break;
    }

    /* sessiond */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.sessiond=0;
        memset(&section->sessiond_addr, 0, sizeof(SOCKADDR_UNION));
        section->sessiond_addr.in.sin_family=AF_INET;
        break;
    case CMD_SET_COPY:
        section->option.sessiond=new_service_options.option.sessiond;
        memcpy(&section->sessiond_addr, &new_service_options.sessiond_addr,
            sizeof(SOCKADDR_UNION));
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sessiond"))
            break;
        section->option.sessiond=1;
#ifdef SSL_OP_NO_TICKET
        /* disable RFC4507 support introduced in OpenSSL 0.9.8f */
        /* this prevents session callbacks from being executed */
        section->ssl_options_set|=SSL_OP_NO_TICKET;
#endif
        if(!name2addr(&section->sessiond_addr, arg, 0))
            return "Failed to resolve sessiond server address";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = [host:]port use sessiond at host:port",
            "sessiond");
        break;
    }

#ifndef OPENSSL_NO_TLSEXT
    /* sni */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->servername_list_head=NULL;
        section->servername_list_tail=NULL;
        break;
    case CMD_SET_COPY:
        section->sni=
            str_dup_detached(new_service_options.sni);
        break;
    case CMD_FREE:
        str_free(section->sni);
        sni_free(section);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sni"))
            break;
        str_free(section->sni);
        section->sni=str_dup_detached(arg);
        return NULL; /* OK */
    case CMD_INITIALIZE:
        {
            char *tmp_str=sni_init(section);
            if(tmp_str)
                return tmp_str;
        }
        if(!section->option.client && section->sni)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = master_service:host_name for an SNI virtual service",
            "sni");
        break;
    }
#endif /* !defined(OPENSSL_NO_TLSEXT) */

    /* socket */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->sock_opts=socket_options_init();
        break;
    case CMD_SET_COPY:
        section->sock_opts=socket_options_dup(new_service_options.sock_opts);
        break;
    case CMD_FREE:
        socket_options_free(section->sock_opts);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "socket"))
            break;
        if(socket_option_parse(section->sock_opts, arg))
            return "Illegal socket option";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = a|l|r:option=value[:value]", "socket");
        s_log(LOG_NOTICE, "%25sset an option on accept/local/remote socket", "");
        break;
    }

#if OPENSSL_VERSION_NUMBER>=0x10100000L

    /* sslVersion */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        /* handled in sslVersionMax and sslVersionMin */
        break;
    case CMD_SET_COPY:
        /* handled in sslVersionMax and sslVersionMin */
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersion"))
            break;
        section->max_proto_version=
            section->min_proto_version=str_to_proto_version(arg);
        if(section->max_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->max_proto_version && section->min_proto_version &&
                section->max_proto_version<section->min_proto_version)
            return "Invalid protocol version range";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersion");
        break;
    }

    /* sslVersionMax */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->max_proto_version=0; /* highest supported */
        break;
    case CMD_SET_COPY:
        section->max_proto_version=new_service_options.max_proto_version;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersionMax"))
            break;
        section->max_proto_version=str_to_proto_version(arg);
        if(section->max_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersionMax");
        break;
    }

    /* sslVersionMin */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->min_proto_version=TLS1_VERSION;
        break;
    case CMD_SET_COPY:
        section->min_proto_version=new_service_options.min_proto_version;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersionMin"))
            break;
        section->min_proto_version=str_to_proto_version(arg);
        if(section->min_proto_version==-1)
            return "Invalid protocol version";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv3|TLSv1|TLSv1.1|TLSv1.2"
#ifdef TLS1_3_VERSION
            "|TLSv1.3"
#endif
            " TLS version", "sslVersionMin");
        break;
    }

#else /* OPENSSL_VERSION_NUMBER<0x10100000L */

    /* sslVersion */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        tls_methods_set(section, NULL);
        break;
    case CMD_SET_COPY:
        section->client_method=new_service_options.client_method;
        section->server_method=new_service_options.server_method;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "sslVersion"))
            break;
        return tls_methods_set(section, arg);
    case CMD_INITIALIZE:
        {
            char *tmp_str=tls_methods_check(section);
            if(tmp_str)
                return tmp_str;
        }
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = all"
            "|SSLv2|SSLv3|TLSv1"
#if OPENSSL_VERSION_NUMBER>=0x10001000L
            "|TLSv1.1|TLSv1.2"
#endif /* OPENSSL_VERSION_NUMBER>=0x10001000L */
            " TLS method", "sslVersion");
        break;
    }

#endif /* OPENSSL_VERSION_NUMBER<0x10100000L */

#ifndef USE_FORK
    /* stack */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->stack_size=DEFAULT_STACK_SIZE;
        break;
    case CMD_SET_COPY:
        section->stack_size=new_service_options.stack_size;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "stack"))
            break;
        {
            char *tmp_str;
            section->stack_size=(size_t)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal thread stack size";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d bytes", "stack", DEFAULT_STACK_SIZE);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = thread stack size (in bytes)", "stack");
        break;
    }
#endif

#if OPENSSL_VERSION_NUMBER>=0x10000000L

    /* ticketKeySecret */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ticket_key=NULL;
        break;
    case CMD_SET_COPY:
        section->ticket_key=key_dup(new_service_options.ticket_key);
        break;
    case CMD_FREE:
        key_free(section->ticket_key);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ticketKeySecret"))
            break;
        section->ticket_key=key_read(arg, "ticketKeySecret");
        if(!section->ticket_key)
            return "Failed to read ticketKeySecret";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ticket_key)          /* ticketKeySecret not configured */
            break;
        if(section->option.client){
            s_log(LOG_NOTICE,
                    "ticketKeySecret is ignored in the client mode");
            break;
        }
        if(section->ticket_key && !section->ticket_mac)
            return "\"ticketKeySecret\" and \"ticketMacSecret\" must be set together";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = secret key for encryption/decryption TLSv1.3 tickets",
            "ticketKeySecret");
        break;
    }

    /* ticketMacSecret */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->ticket_mac=NULL;
        break;
    case CMD_SET_COPY:
        section->ticket_mac=key_dup(new_service_options.ticket_mac);
        break;
    case CMD_FREE:
        key_free(section->ticket_mac);
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "ticketMacSecret"))
            break;
        section->ticket_mac=key_read(arg, "ticketMacSecret");
        if(!section->ticket_mac)
            return "Failed to read ticketMacSecret";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(!section->ticket_mac)            /* ticketMacSecret not configured */
            break;
        if(section->option.client){
            s_log(LOG_NOTICE,
                    "ticketMacSecret is ignored in the client mode");
            break;
        }
        if(section->ticket_mac && !section->ticket_key)
            return "\"ticketKeySecret\" and \"ticketMacSecret\" must be set together";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = key for HMAC operations on TLSv1.3 tickets",
            "ticketMacSecret");
        break;
    }

#endif /* OpenSSL 1.0.0 or later */

    /* TIMEOUTbusy */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_busy=300; /* 5 minutes */
        break;
    case CMD_SET_COPY:
        section->timeout_busy=new_service_options.timeout_busy;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTbusy"))
            break;
        {
            char *tmp_str;
            section->timeout_busy=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal busy timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTbusy", 300);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to wait for expected data", "TIMEOUTbusy");
        break;
    }

    /* TIMEOUTclose */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_close=60; /* 1 minute */
        break;
    case CMD_SET_COPY:
        section->timeout_close=new_service_options.timeout_close;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTclose"))
            break;
        {
            char *tmp_str;
            section->timeout_close=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal close timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTclose", 60);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to wait for close_notify",
            "TIMEOUTclose");
        break;
    }

    /* TIMEOUTconnect */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_connect=10; /* 10 seconds */
        break;
    case CMD_SET_COPY:
        section->timeout_connect=new_service_options.timeout_connect;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTconnect"))
            break;
        {
            char *tmp_str;
            section->timeout_connect=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal connect timeout";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTconnect", 10);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to connect remote host", "TIMEOUTconnect");
        break;
    }

    /* TIMEOUTidle */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->timeout_idle=43200; /* 12 hours */
        break;
    case CMD_SET_COPY:
        section->timeout_idle=new_service_options.timeout_idle;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "TIMEOUTidle"))
            break;
        {
            char *tmp_str;
            section->timeout_idle=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str) /* not a number */
                return "Illegal idle timeout";
            return NULL; /* OK */
        }
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = %d seconds", "TIMEOUTidle", 43200);
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = seconds to keep an idle connection", "TIMEOUTidle");
        break;
    }

    /* transparent */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.transparent_src=0;
        section->option.transparent_dst=0;
        break;
    case CMD_SET_COPY:
        section->option.transparent_src=new_service_options.option.transparent_src;
        section->option.transparent_dst=new_service_options.option.transparent_dst;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "transparent"))
            break;
        if(!strcasecmp(arg, "none") || !strcasecmp(arg, "no")) {
            section->option.transparent_src=0;
            section->option.transparent_dst=0;
        } else if(!strcasecmp(arg, "source") || !strcasecmp(arg, "yes")) {
            section->option.transparent_src=1;
            section->option.transparent_dst=0;
        } else if(!strcasecmp(arg, "destination")) {
            section->option.transparent_src=0;
            section->option.transparent_dst=1;
        } else if(!strcasecmp(arg, "both")) {
            section->option.transparent_src=1;
            section->option.transparent_dst=1;
        } else
            return "Selected transparent proxy mode is not available";
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if(section->option.transparent_dst)
            ++endpoints;
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = none|source|destination|both transparent proxy mode",
            "transparent");
        break;
    }
#endif

    /* verify */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.request_cert=0;
        break;
    case CMD_SET_COPY:
        section->option.request_cert=new_service_options.option.request_cert;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verify"))
            break;
        {
            char *tmp_str;
            int tmp_int=(int)strtol(arg, &tmp_str, 10);
            if(tmp_str==arg || *tmp_str || tmp_int<0 || tmp_int>4)
                return "Bad verify level";
            section->option.request_cert=1;
            section->option.require_cert=(tmp_int>=2);
            section->option.verify_chain=(tmp_int>=1 && tmp_int<=3);
            section->option.verify_peer=(tmp_int>=3);
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        if((section->option.verify_chain || section->option.verify_peer) &&
                !section->ca_file && !section->ca_dir)
            return "Either \"CAfile\" or \"CApath\" has to be configured";
        break;
    case CMD_PRINT_DEFAULTS:
        s_log(LOG_NOTICE, "%-22s = none", "verify");
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE,
            "%-22s = level of peer certificate verification", "verify");
        s_log(LOG_NOTICE,
            "%25slevel 0 - request and ignore peer cert", "");
        s_log(LOG_NOTICE,
            "%25slevel 1 - only validate peer cert if present", "");
        s_log(LOG_NOTICE,
            "%25slevel 2 - always require a valid peer cert", "");
        s_log(LOG_NOTICE,
            "%25slevel 3 - verify peer with locally installed cert", "");
        s_log(LOG_NOTICE,
            "%25slevel 4 - ignore CA chain and only verify peer cert", "");
        break;
    }

    /* verifyChain */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.verify_chain=0;
        break;
    case CMD_SET_COPY:
        section->option.verify_chain=new_service_options.option.verify_chain;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verifyChain"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
            section->option.verify_chain=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.verify_chain=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no verify certificate chain",
            "verifyChain");
        break;
    }

    /* verifyPeer */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        section->option.verify_peer=0;
        break;
    case CMD_SET_COPY:
        section->option.verify_peer=new_service_options.option.verify_peer;
        break;
    case CMD_FREE:
        break;
    case CMD_SET_VALUE:
        if(strcasecmp(opt, "verifyPeer"))
            break;
        if(!strcasecmp(arg, "yes")) {
            section->option.request_cert=1;
            section->option.require_cert=1;
            section->option.verify_peer=1;
        } else if(!strcasecmp(arg, "no")) {
            section->option.verify_peer=0;
        } else {
            return "The argument needs to be either 'yes' or 'no'";
        }
        return NULL; /* OK */
    case CMD_INITIALIZE:
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        s_log(LOG_NOTICE, "%-22s = yes|no verify peer certificate",
            "verifyPeer");
        break;
    }

    /* final checks */
    switch(cmd) {
    case CMD_SET_DEFAULTS:
        break;
    case CMD_SET_COPY:
        break;
    case CMD_FREE:
        str_free(section->chain);
        if(section->session)
            SSL_SESSION_free(section->session);
        if(section->ctx)
            SSL_CTX_free(section->ctx);
        str_free(section->servname);
        if(section==&service_options)
            memset(section, 0, sizeof(SERVICE_OPTIONS));
        else
            str_free(section);
        break;
    case CMD_SET_VALUE:
        return option_not_found;
    case CMD_INITIALIZE:
        if(section!=&new_service_options) { /* daemon mode checks */
            if(endpoints!=2)
                return "Each service must define two endpoints";
        } else { /* inetd mode checks */
            if(section->option.accept)
                return "'accept' option is only allowed in a [section]";
            /* no need to check for section->sni in inetd mode,
               as it requires valid sections to be set */
            if(endpoints!=1)
                return "Inetd mode must define one endpoint";
        }
#ifdef SSL_OP_NO_TICKET
        /* disable RFC4507 support introduced in OpenSSL 0.9.8f */
        /* OpenSSL 1.1.1 is required to serialize application data
         * into session tickets */
        /* server mode sections need it for the "redirect" option
         * and connect address session persistence */
        if(OpenSSL_version_num()<0x10101000L &&
                !section->option.client &&
                !section->option.connect_before_ssl)
            section->ssl_options_set|=SSL_OP_NO_TICKET;
#endif /* SSL_OP_NO_TICKET */
        if(context_init(section)) /* initialize TLS context */
            return "Failed to initialize TLS context";
        break;
    case CMD_PRINT_DEFAULTS:
        break;
    case CMD_PRINT_HELP:
        break;
    }

    return NULL; /* OK */
}