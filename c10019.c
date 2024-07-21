void unbind_ports(void) {
    SERVICE_OPTIONS *opt;

    s_poll_init(fds, 1);

    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_SECTIONS]);

    opt=service_options.next;
    service_options.next=NULL;
    service_free(&service_options);

    while(opt) {
        unsigned i;
        s_log(LOG_DEBUG, "Unbinding service [%s]", opt->servname);
        for(i=0; i<opt->local_addr.num; ++i)
            unbind_port(opt, i);
        /* exec+connect service */
        if(opt->exec_name && opt->connect_addr.names) {
            /* create exec+connect services             */
            /* FIXME: this is just a crude workaround   */
            /*        is it better to kill the service? */
            /* FIXME: this won't work with FORK threads */
            opt->option.retry=0;
        }
        /* purge session cache of the old SSL_CTX object */
        /* this workaround won't be needed anymore after */
        /* delayed deallocation calls SSL_CTX_free()     */
        if(opt->ctx)
            SSL_CTX_flush_sessions(opt->ctx,
                (long)time(NULL)+opt->session_timeout+1);
        s_log(LOG_DEBUG, "Service [%s] closed", opt->servname);

        {
            SERVICE_OPTIONS *garbage=opt;
            opt=opt->next;
            garbage->next=NULL;
            service_free(garbage);
        }
    }

    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_SECTIONS]);
}