static void ev_stream_closed(h2_session *session, h2_stream *stream)
{
    apr_bucket *b;
    
    if (H2_STREAM_CLIENT_INITIATED(stream->id)
        && (stream->id > session->local.completed_max)) {
        session->local.completed_max = stream->id;
    }
    switch (session->state) {
        case H2_SESSION_ST_IDLE:
            if (session->open_streams == 0) {
                /* enter keepalive timeout, since we no longer have streams */
                session->idle_until = (session->s->keep_alive_timeout
                                       + apr_time_now());
            }
            break;
        default:
            break;
    }
    
    /* The stream might have data in the buffers of the main connection.
     * We can only free the allocated resources once all had been written.
     * Send a special buckets on the connection that gets destroyed when
     * all preceding data has been handled. On its destruction, it is safe
     * to purge all resources of the stream. */
    b = h2_bucket_eos_create(session->c->bucket_alloc, stream);
    APR_BRIGADE_INSERT_TAIL(session->bbtmp, b);
    h2_conn_io_pass(&session->io, session->bbtmp);
    apr_brigade_cleanup(session->bbtmp);
}