apr_status_t h2_mplx_idle(h2_mplx *m)
{
    apr_status_t status = APR_SUCCESS;
    apr_time_t now;            
    apr_size_t scount;
    
    H2_MPLX_ENTER(m);

    scount = h2_ihash_count(m->streams);
    if (scount > 0) {
        if (m->tasks_active) {
            /* If we have streams in connection state 'IDLE', meaning
             * all streams are ready to sent data out, but lack
             * WINDOW_UPDATEs. 
             * 
             * This is ok, unless we have streams that still occupy
             * h2 workers. As worker threads are a scarce resource, 
             * we need to take measures that we do not get DoSed.
             * 
             * This is what we call an 'idle block'. Limit the amount 
             * of busy workers we allow for this connection until it
             * well behaves.
             */
            now = apr_time_now();
            m->last_idle_block = now;
            if (m->limit_active > 2 
                && now - m->last_limit_change >= m->limit_change_interval) {
                if (m->limit_active > 16) {
                    m->limit_active = 16;
                }
                else if (m->limit_active > 8) {
                    m->limit_active = 8;
                }
                else if (m->limit_active > 4) {
                    m->limit_active = 4;
                }
                else if (m->limit_active > 2) {
                    m->limit_active = 2;
                }
                m->last_limit_change = now;
                ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                              "h2_mplx(%ld): decrease worker limit to %d",
                              m->id, m->limit_active);
            }
            
            if (m->tasks_active > m->limit_active) {
                status = unschedule_slow_tasks(m);
            }
        }
        else if (!h2_iq_empty(m->q)) {
            ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                          "h2_mplx(%ld): idle, but %d streams to process",
                          m->id, (int)h2_iq_count(m->q));
            status = APR_EAGAIN;
        }
        else {
            /* idle, have streams, but no tasks active. what are we waiting for?
             * WINDOW_UPDATEs from client? */
            h2_stream *stream = NULL;
            
            ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                          "h2_mplx(%ld): idle, no tasks ongoing, %d streams",
                          m->id, (int)h2_ihash_count(m->streams));
            h2_ihash_shift(m->streams, (void**)&stream, 1);
            if (stream) {
                h2_ihash_add(m->streams, stream);
                if (stream->output && !stream->out_checked) {
                    /* FIXME: this looks like a race between the session thinking
                     * it is idle and the EOF on a stream not being sent.
                     * Signal to caller to leave IDLE state.
                     */
                    ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, m->c,
                                  H2_STRM_MSG(stream, "output closed=%d, mplx idle"
                                              ", out has %ld bytes buffered"),
                                  h2_beam_is_closed(stream->output),
                                  (long)h2_beam_get_buffered(stream->output));
                    h2_ihash_add(m->streams, stream);
                    check_data_for(m, stream, 0);
                    stream->out_checked = 1;
                    status = APR_EAGAIN;
                }
            }
        }
    }
    register_if_needed(m);

    H2_MPLX_LEAVE(m);
    return status;
}