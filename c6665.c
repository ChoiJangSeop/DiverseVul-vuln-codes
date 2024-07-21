static void on_stream_state_enter(void *ctx, h2_stream *stream)
{
    h2_session *session = ctx;
    /* stream entered a new state */
    ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                  H2_STRM_MSG(stream, "entered state"));
    switch (stream->state) {
        case H2_SS_IDLE: /* stream was created */
            ++session->open_streams;
            if (H2_STREAM_CLIENT_INITIATED(stream->id)) {
                ++session->remote.emitted_count;
                if (stream->id > session->remote.emitted_max) {
                    session->remote.emitted_max = stream->id;
                    session->local.accepted_max = stream->id;
                }
            }
            else {
                if (stream->id > session->local.emitted_max) {
                    ++session->local.emitted_count;
                    session->remote.emitted_max = stream->id;
                }
            }
            break;
        case H2_SS_OPEN: /* stream has request headers */
        case H2_SS_RSVD_L: /* stream has request headers */
            ev_stream_open(session, stream);
            break;
        case H2_SS_CLOSED_L: /* stream output was closed */
            break;
        case H2_SS_CLOSED_R: /* stream input was closed */
            break;
        case H2_SS_CLOSED: /* stream in+out were closed */
            --session->open_streams;
            ev_stream_closed(session, stream);
            break;
        case H2_SS_CLEANUP:
            h2_mplx_stream_cleanup(session->mplx, stream);
            break;
        default:
            break;
    }
}