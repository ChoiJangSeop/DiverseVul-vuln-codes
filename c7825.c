static apr_status_t unschedule_slow_tasks(h2_mplx *m) 
{
    h2_stream *stream;
    int n;
    
    /* Try to get rid of streams that occupy workers. Look for safe requests
     * that are repeatable. If none found, fail the connection.
     */
    n = (m->tasks_active - m->limit_active - (int)h2_ihash_count(m->sredo));
    while (n > 0 && (stream = get_latest_repeatable_unsubmitted_stream(m))) {
        ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, m->c, 
                      "h2_mplx(%s): unschedule, resetting task for redo later",
                      stream->task->id);
        h2_task_rst(stream->task, H2_ERR_CANCEL);
        h2_ihash_add(m->sredo, stream);
        --n;
    }
    
    if ((m->tasks_active - h2_ihash_count(m->sredo)) > m->limit_active) {
        stream = get_timed_out_busy_stream(m);
        if (stream) {
            /* Too many busy workers, unable to cancel enough streams
             * and with a busy, timed out stream, we tell the client
             * to go away... */
            return APR_TIMEUP;
        }
    }
    return APR_SUCCESS;
}