static void task_done(h2_mplx *m, h2_task *task)
{
    h2_stream *stream;
    
    ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                  "h2_mplx(%ld): task(%s) done", m->id, task->id);
    out_close(m, task);
    
    task->worker_done = 1;
    task->done_at = apr_time_now();
    ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, m->c,
                  "h2_mplx(%s): request done, %f ms elapsed", task->id, 
                  (task->done_at - task->started_at) / 1000.0);
    
    if (task->started_at > m->last_idle_block) {
        /* this task finished without causing an 'idle block', e.g.
         * a block by flow control.
         */
        if (task->done_at- m->last_limit_change >= m->limit_change_interval
            && m->limit_active < m->max_active) {
            /* Well behaving stream, allow it more workers */
            m->limit_active = H2MIN(m->limit_active * 2, 
                                     m->max_active);
            m->last_limit_change = task->done_at;
            ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, m->c,
                          "h2_mplx(%ld): increase worker limit to %d",
                          m->id, m->limit_active);
        }
    }

    ap_assert(task->done_done == 0);

    stream = h2_ihash_get(m->streams, task->stream_id);
    if (stream) {
        /* stream not done yet. */
        if (!m->aborted && h2_ihash_get(m->sredo, stream->id)) {
            /* reset and schedule again */
            task->worker_done = 0;
            h2_task_redo(task);
            h2_ihash_remove(m->sredo, stream->id);
            h2_iq_add(m->q, stream->id, NULL, NULL);
            ap_log_cerror(APLOG_MARK, APLOG_INFO, 0, m->c,
                          H2_STRM_MSG(stream, "redo, added to q")); 
        }
        else {
            /* stream not cleaned up, stay around */
            task->done_done = 1;
            ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, m->c,
                          H2_STRM_MSG(stream, "task_done, stream open")); 
            if (stream->input) {
                h2_beam_leave(stream->input);
            }

            /* more data will not arrive, resume the stream */
            check_data_for(m, stream, 0);            
        }
    }
    else if ((stream = h2_ihash_get(m->shold, task->stream_id)) != NULL) {
        /* stream is done, was just waiting for this. */
        task->done_done = 1;
        ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, m->c,
                      H2_STRM_MSG(stream, "task_done, in hold"));
        if (stream->input) {
            h2_beam_leave(stream->input);
        }
        stream_joined(m, stream);
    }
    else if ((stream = h2_ihash_get(m->spurge, task->stream_id)) != NULL) {
        ap_log_cerror(APLOG_MARK, APLOG_WARNING, 0, m->c,   
                      H2_STRM_LOG(APLOGNO(03517), stream, "already in spurge"));
        ap_assert("stream should not be in spurge" == NULL);
    }
    else {
        ap_log_cerror(APLOG_MARK, APLOG_WARNING, 0, m->c, APLOGNO(03518)
                      "h2_mplx(%s): task_done, stream not found", 
                      task->id);
        ap_assert("stream should still be available" == NULL);
    }
}