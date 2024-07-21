NOEXPORT void reload_config() {
    static int delay=10; /* 10ms */
#ifdef HAVE_CHROOT
    struct stat sb;
#endif /* HAVE_CHROOT */

    if(options_parse(CONF_RELOAD)) {
        s_log(LOG_ERR, "Failed to reload the configuration file");
        return;
    }
    unbind_ports();
    log_flush(LOG_MODE_BUFFER);
#ifdef HAVE_CHROOT
    /* we don't close SINK_SYSLOG if chroot is enabled and
     * there is no /dev/log inside it, which could allow
     * openlog(3) to reopen the syslog socket later */
    if(global_options.chroot_dir && stat("/dev/log", &sb))
        log_close(SINK_OUTFILE);
    else
#endif /* HAVE_CHROOT */
        log_close(SINK_SYSLOG|SINK_OUTFILE);
    /* there is no race condition here:
     * client threads are not allowed to use global options */
    options_free();
    options_apply();
    /* we hope that a sane openlog(3) implementation won't
     * attempt to reopen /dev/log if it's already open */
    log_open(SINK_SYSLOG|SINK_OUTFILE);
    log_flush(LOG_MODE_CONFIGURED);
    ui_config_reloaded();
    /* we use "|" instead of "||" to attempt initialization of both subsystems */
    if(bind_ports() | exec_connect_start()) {
        s_poll_sleep(delay/1000, delay%1000); /* sleep to avoid log trashing */
        signal_post(SIGNAL_RELOAD_CONFIG); /* retry */
        delay*=2;
        if(delay > 10000) /* 10s */
            delay=10000;
    } else {
        delay=10; /* 10ms */
    }
}