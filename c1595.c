main(const int argc, char **argv)
{
    int                 n_listeners, i, clnt_length, clnt;
    struct pollfd       *polls;
    LISTENER            *lstn;
    pthread_t           thr;
    pthread_attr_t      attr;
    struct sched_param  sp;
    uid_t               user_id;
    gid_t               group_id;
    FILE                *fpid;
    struct sockaddr_storage clnt_addr;
    char                tmp[MAXBUF];
#ifndef SOL_TCP
    struct protoent     *pe;
#endif

    print_log = 0;
    (void)umask(077);
    control_sock = -1;
    log_facility = -1;
    logmsg(LOG_NOTICE, "starting...");

    signal(SIGHUP, h_shut);
    signal(SIGINT, h_shut);
    signal(SIGTERM, h_term);
    signal(SIGQUIT, h_term);
    signal(SIGPIPE, SIG_IGN);

    srandom(getpid());

    /* SSL stuff */
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    l_init();
    init_thr_arg();
    CRYPTO_set_id_callback(l_id);
    CRYPTO_set_locking_callback(l_lock);
    init_timer();

    /* prepare regular expressions */
    if(regcomp(&HEADER, "^([a-z0-9!#$%&'*+.^_`|~-]+):[ \t]*(.*)[ \t]*$", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    || regcomp(&CHUNK_HEAD, "^([0-9a-f]+).*$", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    || regcomp(&RESP_SKIP, "^HTTP/1.1 100.*$", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    || regcomp(&RESP_IGN, "^HTTP/1.[01] (10[1-9]|1[1-9][0-9]|204|30[456]).*$", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    || regcomp(&LOCATION, "(http|https)://([^/]+)(.*)", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    || regcomp(&AUTHORIZATION, "Authorization:[ \t]*Basic[ \t]*\"?([^ \t]*)\"?[ \t]*", REG_ICASE | REG_NEWLINE | REG_EXTENDED)
    ) {
        logmsg(LOG_ERR, "bad essential Regex - aborted");
        exit(1);
    }

#ifndef SOL_TCP
    /* for systems without the definition */
    if((pe = getprotobyname("tcp")) == NULL) {
        logmsg(LOG_ERR, "missing TCP protocol");
        exit(1);
    }
    SOL_TCP = pe->p_proto;
#endif

    /* read config */
    config_parse(argc, argv);

    if(log_facility != -1)
        openlog("pound", LOG_CONS | LOG_NDELAY, LOG_DAEMON);
    if(ctrl_name != NULL) {
        struct sockaddr_un  ctrl;

        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.sun_family = AF_UNIX;
        strncpy(ctrl.sun_path, ctrl_name, sizeof(ctrl.sun_path) - 1);
        (void)unlink(ctrl.sun_path);
        if((control_sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
            logmsg(LOG_ERR, "Control \"%s\" create: %s", ctrl.sun_path, strerror(errno));
            exit(1);
        }
        if(bind(control_sock, (struct sockaddr *)&ctrl, (socklen_t)sizeof(ctrl)) < 0) {
            logmsg(LOG_ERR, "Control \"%s\" bind: %s", ctrl.sun_path, strerror(errno));
            exit(1);
        }
        listen(control_sock, 512);
    }

    /* open listeners */
    for(lstn = listeners, n_listeners = 0; lstn; lstn = lstn->next, n_listeners++) {
        int opt;

        /* prepare the socket */
        if((lstn->sock = socket(lstn->addr.ai_family == AF_INET? PF_INET: PF_INET6, SOCK_STREAM, 0)) < 0) {
            addr2str(tmp, MAXBUF - 1, &lstn->addr, 0);
            logmsg(LOG_ERR, "HTTP socket %s create: %s - aborted", tmp, strerror(errno));
            exit(1);
        }
        opt = 1;
        setsockopt(lstn->sock, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));
        if(bind(lstn->sock, lstn->addr.ai_addr, (socklen_t)lstn->addr.ai_addrlen) < 0) {
            addr2str(tmp, MAXBUF - 1, &lstn->addr, 0);
            logmsg(LOG_ERR, "HTTP socket bind %s: %s - aborted", tmp, strerror(errno));
            exit(1);
        }
        listen(lstn->sock, 512);
    }

    /* alloc the poll structures */
    if((polls = (struct pollfd *)calloc(n_listeners, sizeof(struct pollfd))) == NULL) {
        logmsg(LOG_ERR, "Out of memory for poll - aborted");
        exit(1);
    }
    for(lstn = listeners, i = 0; lstn; lstn = lstn->next, i++)
        polls[i].fd = lstn->sock;

    /* set uid if necessary */
    if(user) {
        struct passwd   *pw;

        if((pw = getpwnam(user)) == NULL) {
            logmsg(LOG_ERR, "no such user %s - aborted", user);
            exit(1);
        }
        user_id = pw->pw_uid;
    }

    /* set gid if necessary */
    if(group) {
        struct group    *gr;

        if((gr = getgrnam(group)) == NULL) {
            logmsg(LOG_ERR, "no such group %s - aborted", group);
            exit(1);
        }
        group_id = gr->gr_gid;
    }

    /* Turn off verbose messages (if necessary) */
    print_log = 0;

    if(daemonize) {
        /* daemonize - make ourselves a subprocess. */
        switch (fork()) {
            case 0:
                if(log_facility != -1) {
                    close(0);
                    close(1);
                    close(2);
                }
                break;
            case -1:
                logmsg(LOG_ERR, "fork: %s - aborted", strerror(errno));
                exit(1);
            default:
                exit(0);
        }
#ifdef  HAVE_SETSID
        (void) setsid();
#endif
    }

    /* record pid in file */
    if((fpid = fopen(pid_name, "wt")) != NULL) {
        fprintf(fpid, "%d\n", getpid());
        fclose(fpid);
    } else
        logmsg(LOG_NOTICE, "Create \"%s\": %s", pid_name, strerror(errno));

    /* chroot if necessary */
    if(root_jail) {
        if(chroot(root_jail)) {
            logmsg(LOG_ERR, "chroot: %s - aborted", strerror(errno));
            exit(1);
        }
        if(chdir("/")) {
            logmsg(LOG_ERR, "chroot/chdir: %s - aborted", strerror(errno));
            exit(1);
        }
    }

    if(group)
        if(setgid(group_id) || setegid(group_id)) {
            logmsg(LOG_ERR, "setgid: %s - aborted", strerror(errno));
            exit(1);
        }
    if(user)
        if(setuid(user_id) || seteuid(user_id)) {
            logmsg(LOG_ERR, "setuid: %s - aborted", strerror(errno));
            exit(1);
        }

    /* split off into monitor and working process if necessary */
    for(;;) {
#ifdef  UPER
        if((son = fork()) > 0) {
            int status;

            (void)wait(&status);
            if(WIFEXITED(status))
                logmsg(LOG_ERR, "MONITOR: worker exited normally %d, restarting...", WEXITSTATUS(status));
            else if(WIFSIGNALED(status))
                logmsg(LOG_ERR, "MONITOR: worker exited on signal %d, restarting...", WTERMSIG(status));
            else
                logmsg(LOG_ERR, "MONITOR: worker exited (stopped?) %d, restarting...", status);
        } else if (son == 0) {
#endif

            /* thread stuff */
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

#ifdef  NEED_STACK
            /* set new stack size - necessary for OpenBSD/FreeBSD and Linux NPTL */
            if(pthread_attr_setstacksize(&attr, 1 << 18)) {
                logmsg(LOG_ERR, "can't set stack size - aborted");
                exit(1);
            }
#endif
            /* start timer */
            if(pthread_create(&thr, &attr, thr_timer, NULL)) {
                logmsg(LOG_ERR, "create thr_resurect: %s - aborted", strerror(errno));
                exit(1);
            }

            /* start the controlling thread (if needed) */
            if(control_sock >= 0 && pthread_create(&thr, &attr, thr_control, NULL)) {
                logmsg(LOG_ERR, "create thr_control: %s - aborted", strerror(errno));
                exit(1);
            }

            /* pause to make sure the service threads were started */
            sleep(1);

            /* create the worker threads */
            for(i = 0; i < numthreads; i++)
                if(pthread_create(&thr, &attr, thr_http, NULL)) {
                    logmsg(LOG_ERR, "create thr_http: %s - aborted", strerror(errno));
                    exit(1);
                }

            /* pause to make sure at least some of the worker threads were started */
            sleep(1);

            /* and start working */
            for(;;) {
                if(shut_down) {
                    logmsg(LOG_NOTICE, "shutting down...");
                    for(lstn = listeners; lstn; lstn = lstn->next)
                        close(lstn->sock);
                    if(grace > 0) {
                        sleep(grace);
                        logmsg(LOG_NOTICE, "grace period expired - exiting...");
                    }
                    if(ctrl_name != NULL)
                        (void)unlink(ctrl_name);
                    exit(0);
                }
                for(lstn = listeners, i = 0; i < n_listeners; lstn = lstn->next, i++) {
                    polls[i].events = POLLIN | POLLPRI;
                    polls[i].revents = 0;
                }
                if(poll(polls, n_listeners, -1) < 0) {
                    logmsg(LOG_WARNING, "poll: %s", strerror(errno));
                } else {
                    for(lstn = listeners, i = 0; lstn; lstn = lstn->next, i++) {
                        if(polls[i].revents & (POLLIN | POLLPRI)) {
                            memset(&clnt_addr, 0, sizeof(clnt_addr));
                            clnt_length = sizeof(clnt_addr);
                            if((clnt = accept(lstn->sock, (struct sockaddr *)&clnt_addr,
                                (socklen_t *)&clnt_length)) < 0) {
                                logmsg(LOG_WARNING, "HTTP accept: %s", strerror(errno));
                            } else if(((struct sockaddr_in *)&clnt_addr)->sin_family == AF_INET
                                   || ((struct sockaddr_in *)&clnt_addr)->sin_family == AF_INET6) {
                                thr_arg arg;

                                if(lstn->disabled) {
                                    /*
                                    addr2str(tmp, MAXBUF - 1, &clnt_addr, 1);
                                    logmsg(LOG_WARNING, "HTTP disabled listener from %s", tmp);
                                    */
                                    close(clnt);
                                }
                                arg.sock = clnt;
                                arg.lstn = lstn;
                                if((arg.from_host.ai_addr = (struct sockaddr *)malloc(clnt_length)) == NULL) {
                                    logmsg(LOG_WARNING, "HTTP arg address: malloc");
                                    close(clnt);
                                    continue;
                                }
                                memcpy(arg.from_host.ai_addr, &clnt_addr, clnt_length);
                                arg.from_host.ai_addrlen = clnt_length;
                                if(((struct sockaddr_in *)&clnt_addr)->sin_family == AF_INET)
                                    arg.from_host.ai_family = AF_INET;
                                else
                                    arg.from_host.ai_family = AF_INET6;
                                if(put_thr_arg(&arg))
                                    close(clnt);
                            } else {
                                /* may happen on FreeBSD, I am told */
                                logmsg(LOG_WARNING, "HTTP connection prematurely closed by peer");
                                close(clnt);
                            }
                        }
                    }
                }
            }
#ifdef  UPER
        } else {
            /* failed to spawn son */
            logmsg(LOG_ERR, "Can't fork worker (%s) - aborted", strerror(errno));
            exit(1);
        }
#endif
    }
}