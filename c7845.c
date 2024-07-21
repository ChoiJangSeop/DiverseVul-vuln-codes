static h2_task *next_stream_task(h2_mplx *m)
{
    h2_stream *stream;
    int sid;
    while (!m->aborted && (m->tasks_active < m->limit_active)
           && (sid = h2_iq_shift(m->q)) > 0) {
        
        stream = h2_ihash_get(m->streams, sid);
        if (stream) {
            conn_rec *slave, **pslave;

            pslave = (conn_rec **)apr_array_pop(m->spare_slaves);
            if (pslave) {
                slave = *pslave;
                slave->aborted = 0;
            }
            else {
                slave = h2_slave_create(m->c, stream->id, m->pool);
            }
            
            if (!stream->task) {

                if (sid > m->max_stream_started) {
                    m->max_stream_started = sid;
                }
                if (stream->input) {
                    h2_beam_on_consumed(stream->input, stream_input_ev, 
                                        stream_input_consumed, stream);
                }
                
                stream->task = h2_task_create(slave, stream->id, 
                                              stream->request, m, stream->input, 
                                              stream->session->s->timeout,
                                              m->stream_max_mem);
                if (!stream->task) {
                    ap_log_cerror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, slave,
                                  H2_STRM_LOG(APLOGNO(02941), stream, 
                                  "create task"));
                    return NULL;
                }
                
            }
            
            ++m->tasks_active;
            return stream->task;
        }
    }
    return NULL;
}