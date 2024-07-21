void log_flush(LOG_MODE new_mode) {
    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_LOG_MODE]);

    /* prevent changing LOG_MODE_CONFIGURED to LOG_MODE_ERROR
     * once stderr file descriptor is closed */
    if(log_mode!=LOG_MODE_CONFIGURED || new_mode!=LOG_MODE_ERROR)
        log_mode=new_mode;

    /* emit the buffered logs (unless we just started buffering) */
    if(new_mode!=LOG_MODE_BUFFER) {
        /* log_raw() will use the new value of log_mode */
        CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_LOG_BUFFER]);
        while(head) {
            struct LIST *tmp=head;
            head=head->next;
            log_raw(tmp->opt, tmp->level, tmp->stamp, tmp->id, tmp->text);
            str_free(tmp);
        }
        head=tail=NULL;
        CRYPTO_THREAD_unlock(stunnel_locks[LOCK_LOG_BUFFER]);
    }

    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_LOG_MODE]);
}