int main_configure(char *arg1, char *arg2) {
    int cmdline_status;

    cmdline_status=options_cmdline(arg1, arg2);
    if(cmdline_status) /* cannot proceed */
        return cmdline_status;
    options_apply();
    str_canary_init(); /* needs prng initialization from options_cmdline */
    /* log_open(SINK_SYSLOG) must be called before change_root()
     * to be able to access /dev/log socket */
    log_open(SINK_SYSLOG);
    if(bind_ports())
        return 1;

#ifdef HAVE_CHROOT
    /* change_root() must be called before drop_privileges()
     * since chroot() needs root privileges */
    if(change_root())
        return 1;
#endif /* HAVE_CHROOT */

    if(drop_privileges(1))
        return 1;

    /* log_open(SINK_OUTFILE) must be called after drop_privileges()
     * or logfile rotation won't be possible */
    if(log_open(SINK_OUTFILE))
        return 1;
#ifndef USE_FORK
    num_clients=0; /* the first valid config */
#endif
    /* log_flush(LOG_MODE_CONFIGURED) must be called before daemonize()
     * since daemonize() invalidates stderr */
    log_flush(LOG_MODE_CONFIGURED);
    return 0;
}