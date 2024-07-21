void main_init() { /* one-time initialization */
#ifdef USE_SYSTEMD
    int i;

    systemd_fds=sd_listen_fds(1);
    if(systemd_fds<0)
        fatal("systemd initialization failed");
    listen_fds_start=SD_LISTEN_FDS_START;
    /* set non-blocking mode on systemd file descriptors */
    for(i=0; i<systemd_fds; ++i)
        set_nonblock(listen_fds_start+i, 1);
#else
    systemd_fds=0; /* no descriptors received */
    listen_fds_start=3; /* the value is not really important */
#endif
    /* basic initialization contains essential functions required for logging
     * subsystem to function properly, thus all errors here are fatal */
    if(ssl_init()) /* initialize TLS library */
        fatal("TLS initialization failed");
    if(sthreads_init()) /* initialize critical sections & TLS callbacks */
        fatal("Threads initialization failed");
    options_defaults();
    options_apply();
#ifndef USE_FORK
    get_limits(); /* required by setup_fd() */
#endif
    fds=s_poll_alloc();
    if(pipe_init(signal_pipe, "signal_pipe"))
        fatal("Signal pipe initialization failed: "
            "check your personal firewall");
    if(pipe_init(terminate_pipe, "terminate_pipe"))
        fatal("Terminate pipe initialization failed: "
            "check your personal firewall");
    stunnel_info(LOG_NOTICE);
    if(systemd_fds>0)
        s_log(LOG_INFO, "Systemd socket activation: %d descriptors received",
            systemd_fds);
}