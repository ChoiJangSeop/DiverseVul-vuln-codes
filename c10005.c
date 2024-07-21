void main_cleanup() {
#ifdef USE_OS_THREADS
    CLI *c;
    unsigned i, threads;
    THREAD_ID *thread_list;

    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_THREAD_LIST]);
    threads=0;
    for(c=thread_head; c; c=c->thread_next) /* count client threads */
        threads++;
    thread_list=str_alloc((threads+1)*sizeof(THREAD_ID));
    i=0;
    for(c=thread_head; c; c=c->thread_next) { /* copy client threads */
        thread_list[i++]=c->thread_id;
        s_log(LOG_DEBUG, "Terminating a thread for [%s]", c->opt->servname);
    }
    if(cron_thread_id) { /* append cron_thread_id if used */
        thread_list[threads++]=cron_thread_id;
        s_log(LOG_DEBUG, "Terminating the cron thread");
    }
    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_THREAD_LIST]);

    if(threads) {
        s_log(LOG_NOTICE, "Terminating %u service thread(s)", threads);
        writesocket(terminate_pipe[1], "", 1);
        for(i=0; i<threads; ++i) { /* join client threads */
#ifdef USE_PTHREAD
            if(pthread_join(thread_list[i], NULL))
                s_log(LOG_ERR, "pthread_join() failed");
#endif
#ifdef USE_WIN32
            if(WaitForSingleObject(thread_list[i], INFINITE)==WAIT_FAILED)
                ioerror("WaitForSingleObject");
            if(!CloseHandle(thread_list[i]))
                ioerror("CloseHandle");
#endif
        }
        s_log(LOG_NOTICE, "Service threads terminated");
    }

    str_free(thread_list);
#endif /* USE_OS_THREADS */

    unbind_ports();
    s_poll_free(fds);
    fds=NULL;
#if 0
    str_stats(); /* main thread allocation tracking */
#endif
    log_flush(LOG_MODE_ERROR);
    log_close(SINK_SYSLOG|SINK_OUTFILE);
}