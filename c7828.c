apr_status_t h2_session_process(h2_session *session, int async)
{
    apr_status_t status = APR_SUCCESS;
    conn_rec *c = session->c;
    int rv, mpm_state, trace = APLOGctrace3(c);
    apr_time_t now;
    
    if (trace) {
        ap_log_cerror( APLOG_MARK, APLOG_TRACE3, status, c,
                      H2_SSSN_MSG(session, "process start, async=%d"), async);
    }
                  
    while (session->state != H2_SESSION_ST_DONE) {
        now = apr_time_now();
        session->have_read = session->have_written = 0;

        if (session->local.accepting 
            && !ap_mpm_query(AP_MPMQ_MPM_STATE, &mpm_state)) {
            if (mpm_state == AP_MPMQ_STOPPING) {
                dispatch_event(session, H2_SESSION_EV_MPM_STOPPING, 0, NULL);
            }
        }
        
        session->status[0] = '\0';
        
        switch (session->state) {
            case H2_SESSION_ST_INIT:
                ap_update_child_status_from_conn(c->sbh, SERVER_BUSY_READ, c);
                if (!h2_is_acceptable_connection(c, session->r, 1)) {
                    update_child_status(session, SERVER_BUSY_READ, 
                                        "inadequate security");
                    h2_session_shutdown(session, 
                                        NGHTTP2_INADEQUATE_SECURITY, NULL, 1);
                } 
                else {
                    update_child_status(session, SERVER_BUSY_READ, "init");
                    status = h2_session_start(session, &rv);
                    ap_log_cerror(APLOG_MARK, APLOG_DEBUG, status, c, 
                                  H2_SSSN_LOG(APLOGNO(03079), session, 
                                  "started on %s:%d"), 
                                  session->s->server_hostname,
                                  c->local_addr->port);
                    if (status != APR_SUCCESS) {
                        dispatch_event(session, 
                                       H2_SESSION_EV_CONN_ERROR, 0, NULL);
                    }
                    dispatch_event(session, H2_SESSION_EV_INIT, 0, NULL);
                }
                break;
                
            case H2_SESSION_ST_IDLE:
                if (session->idle_until && (apr_time_now() + session->idle_delay) > session->idle_until) {
                    ap_log_cerror( APLOG_MARK, APLOG_TRACE1, status, c,
                                  H2_SSSN_MSG(session, "idle, timeout reached, closing"));
                    if (session->idle_delay) {
                        apr_table_setn(session->c->notes, "short-lingering-close", "1"); 
                    }
                    dispatch_event(session, H2_SESSION_EV_CONN_TIMEOUT, 0, "timeout");
                    goto out;
                }
                
                if (session->idle_delay) {
                    /* we are less interested in spending time on this connection */
                    ap_log_cerror( APLOG_MARK, APLOG_TRACE2, status, c,
                                  H2_SSSN_MSG(session, "session is idle (%ld ms), idle wait %ld sec left"), 
                                  (long)apr_time_as_msec(session->idle_delay),
                                  (long)apr_time_sec(session->idle_until - now));
                    apr_sleep(session->idle_delay);
                    session->idle_delay = 0;
                }

                h2_conn_io_flush(&session->io);
                if (async && !session->r && (now > session->idle_sync_until)) {
                    if (trace) {
                        ap_log_cerror(APLOG_MARK, APLOG_TRACE3, status, c,
                                      H2_SSSN_MSG(session, 
                                      "nonblock read, %d streams open"), 
                                      session->open_streams);
                    }
                    status = h2_session_read(session, 0);
                    
                    if (status == APR_SUCCESS) {
                        session->have_read = 1;
                    }
                    else if (APR_STATUS_IS_EAGAIN(status) || APR_STATUS_IS_TIMEUP(status)) {
                        status = APR_EAGAIN;
                        goto out;
                    }
                    else {
                        ap_log_cerror(APLOG_MARK, APLOG_DEBUG, status, c,
                                      H2_SSSN_LOG(APLOGNO(03403), session, 
                                      "no data, error"));
                        dispatch_event(session, 
                                       H2_SESSION_EV_CONN_ERROR, 0, "timeout");
                    }
                }
                else {
                    /* make certain, we send everything before we idle */
                    if (trace) {
                        ap_log_cerror(APLOG_MARK, APLOG_TRACE3, status, c,
                                      H2_SSSN_MSG(session, 
                                      "sync, stutter 1-sec, %d streams open"), 
                                      session->open_streams);
                    }
                    /* We wait in smaller increments, using a 1 second timeout.
                     * That gives us the chance to check for MPMQ_STOPPING often. 
                     */
                    status = h2_mplx_idle(session->mplx);
                    if (status == APR_EAGAIN) {
                        break;
                    }
                    else if (status != APR_SUCCESS) {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 
                                       H2_ERR_ENHANCE_YOUR_CALM, "less is more");
                    }
                    h2_filter_cin_timeout_set(session->cin, apr_time_from_sec(1));
                    status = h2_session_read(session, 1);
                    if (status == APR_SUCCESS) {
                        session->have_read = 1;
                    }
                    else if (status == APR_EAGAIN) {
                        /* nothing to read */
                    }
                    else if (APR_STATUS_IS_TIMEUP(status)) {
                        /* continue reading handling */
                    }
                    else if (APR_STATUS_IS_ECONNABORTED(status)
                             || APR_STATUS_IS_ECONNRESET(status)
                             || APR_STATUS_IS_EOF(status)
                             || APR_STATUS_IS_EBADF(status)) {
                        ap_log_cerror( APLOG_MARK, APLOG_TRACE3, status, c,
                                      H2_SSSN_MSG(session, "input gone"));
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
                    }
                    else {
                        ap_log_cerror( APLOG_MARK, APLOG_TRACE3, status, c,
                                      H2_SSSN_MSG(session, 
                                      "(1 sec timeout) read failed"));
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, "error");
                    }
                }
                if (nghttp2_session_want_write(session->ngh2)) {
                    ap_update_child_status(session->c->sbh, SERVER_BUSY_WRITE, NULL);
                    status = h2_session_send(session);
                    if (status == APR_SUCCESS) {
                        status = h2_conn_io_flush(&session->io);
                    }
                    if (status != APR_SUCCESS) {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 
                                       H2_ERR_INTERNAL_ERROR, "writing");
                        break;
                    }
                }
                break;
                
            case H2_SESSION_ST_BUSY:
                if (nghttp2_session_want_read(session->ngh2)) {
                    ap_update_child_status(session->c->sbh, SERVER_BUSY_READ, NULL);
                    h2_filter_cin_timeout_set(session->cin, session->s->timeout);
                    status = h2_session_read(session, 0);
                    if (status == APR_SUCCESS) {
                        session->have_read = 1;
                    }
                    else if (status == APR_EAGAIN) {
                        /* nothing to read */
                    }
                    else if (APR_STATUS_IS_TIMEUP(status)) {
                        dispatch_event(session, H2_SESSION_EV_CONN_TIMEOUT, 0, NULL);
                        break;
                    }
                    else {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
                    }
                }

                status = dispatch_master(session);
                if (status != APR_SUCCESS && status != APR_EAGAIN) {
                    break;
                }
                
                if (nghttp2_session_want_write(session->ngh2)) {
                    ap_update_child_status(session->c->sbh, SERVER_BUSY_WRITE, NULL);
                    status = h2_session_send(session);
                    if (status == APR_SUCCESS) {
                        status = h2_conn_io_flush(&session->io);
                    }
                    if (status != APR_SUCCESS) {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 
                                       H2_ERR_INTERNAL_ERROR, "writing");
                        break;
                    }
                }
                
                if (session->have_read || session->have_written) {
                    if (session->wait_us) {
                        session->wait_us = 0;
                    }
                }
                else if (!nghttp2_session_want_write(session->ngh2)) {
                    dispatch_event(session, H2_SESSION_EV_NO_IO, 0, NULL);
                }
                break;
                
            case H2_SESSION_ST_WAIT:
                if (session->wait_us <= 0) {
                    session->wait_us = 10;
                    if (h2_conn_io_flush(&session->io) != APR_SUCCESS) {
                        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
                        break;
                    }
                }
                else {
                    /* repeating, increase timer for graceful backoff */
                    session->wait_us = H2MIN(session->wait_us*2, MAX_WAIT_MICROS);
                }

                if (trace) {
                    ap_log_cerror(APLOG_MARK, APLOG_TRACE3, 0, c,
                                  "h2_session: wait for data, %ld micros", 
                                  (long)session->wait_us);
                }
                status = h2_mplx_out_trywait(session->mplx, session->wait_us, 
                                             session->iowait);
                if (status == APR_SUCCESS) {
                    session->wait_us = 0;
                        dispatch_event(session, H2_SESSION_EV_STREAM_CHANGE, 0, NULL);
                }
                else if (APR_STATUS_IS_TIMEUP(status)) {
                    /* go back to checking all inputs again */
                    transit(session, "wait cycle", session->local.shutdown? 
                            H2_SESSION_ST_DONE : H2_SESSION_ST_BUSY);
                }
                else if (APR_STATUS_IS_ECONNRESET(status) 
                         || APR_STATUS_IS_ECONNABORTED(status)) {
                    dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
                }
                else {
                    ap_log_cerror(APLOG_MARK, APLOG_WARNING, status, c,
                                  H2_SSSN_LOG(APLOGNO(03404), session, 
                                  "waiting on conditional"));
                    h2_session_shutdown(session, H2_ERR_INTERNAL_ERROR, 
                                        "cond wait error", 0);
                }
                break;
                
            default:
                ap_log_cerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, c,
                              H2_SSSN_LOG(APLOGNO(03080), session, 
                              "unknown state"));
                dispatch_event(session, H2_SESSION_EV_PROTO_ERROR, 0, NULL);
                break;
        }

        if (!nghttp2_session_want_read(session->ngh2) 
                 && !nghttp2_session_want_write(session->ngh2)) {
            dispatch_event(session, H2_SESSION_EV_NGH2_DONE, 0, NULL); 
        }
        if (session->reprioritize) {
            h2_mplx_reprioritize(session->mplx, stream_pri_cmp, session);
            session->reprioritize = 0;
        }
    }
    
out:
    if (trace) {
        ap_log_cerror( APLOG_MARK, APLOG_TRACE3, status, c,
                      H2_SSSN_MSG(session, "process returns")); 
    }
    
    if ((session->state != H2_SESSION_ST_DONE)
        && (APR_STATUS_IS_EOF(status)
            || APR_STATUS_IS_ECONNRESET(status) 
            || APR_STATUS_IS_ECONNABORTED(status))) {
        dispatch_event(session, H2_SESSION_EV_CONN_ERROR, 0, NULL);
    }

    return (session->state == H2_SESSION_ST_DONE)? APR_EOF : APR_SUCCESS;
}