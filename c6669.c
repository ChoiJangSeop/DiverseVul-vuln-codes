static void ev_stream_open(h2_session *session, h2_stream *stream)
{
    h2_iq_append(session->in_process, stream->id);
    switch (session->state) {
        case H2_SESSION_ST_IDLE:
            if (session->open_streams == 1) {
                /* enter timeout, since we have a stream again */
                session->idle_until = (session->s->timeout + apr_time_now());
            }
            break;
        default:
            break;
    }
}