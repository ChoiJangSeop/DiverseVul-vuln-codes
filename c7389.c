void h2_mplx_release_and_join(h2_mplx *m, apr_thread_cond_t *wait)
{
    apr_status_t status;
    int i, wait_secs = 60;

    /* How to shut down a h2 connection:
     * 0. abort and tell the workers that no more tasks will come from us */
    m->aborted = 1;
    h2_workers_unregister(m->workers, m);
    
    H2_MPLX_ENTER_ALWAYS(m);

    /* How to shut down a h2 connection:
     * 1. cancel all streams still active */
    while (!h2_ihash_iter(m->streams, stream_cancel_iter, m)) {
        /* until empty */
    }
    
    /* 2. terminate ngn_shed, no more streams
     * should be scheduled or in the active set */
    h2_ngn_shed_abort(m->ngn_shed);
    ap_assert(h2_ihash_empty(m->streams));
    ap_assert(h2_iq_empty(m->q));
    
    /* 3. while workers are busy on this connection, meaning they
     *    are processing tasks from this connection, wait on them finishing
     *    in order to wake us and let us check again. 
     *    Eventually, this has to succeed. */    
    m->join_wait = wait;
    for (i = 0; h2_ihash_count(m->shold) > 0; ++i) {        
        status = apr_thread_cond_timedwait(wait, m->lock, apr_time_from_sec(wait_secs));
        
        if (APR_STATUS_IS_TIMEUP(status)) {
            /* This can happen if we have very long running requests
             * that do not time out on IO. */
            ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, m->c, APLOGNO(03198)
                          "h2_mplx(%ld): waited %d sec for %d tasks", 
                          m->id, i*wait_secs, (int)h2_ihash_count(m->shold));
            h2_ihash_iter(m->shold, report_stream_iter, m);
        }
    }
    ap_assert(m->tasks_active == 0);
    m->join_wait = NULL;
    
    /* 4. close the h2_req_enginge shed */
    h2_ngn_shed_destroy(m->ngn_shed);
    m->ngn_shed = NULL;
    
    /* 4. With all workers done, all streams should be in spurge */
    if (!h2_ihash_empty(m->shold)) {
        ap_log_cerror(APLOG_MARK, APLOG_WARNING, 0, m->c, APLOGNO(03516)
                      "h2_mplx(%ld): unexpected %d streams in hold", 
                      m->id, (int)h2_ihash_count(m->shold));
        h2_ihash_iter(m->shold, unexpected_stream_iter, m);
    }
    
    H2_MPLX_LEAVE(m);

    ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                  "h2_mplx(%ld): released", m->id);
}