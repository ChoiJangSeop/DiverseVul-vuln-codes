static int on_frame_recv_cb(nghttp2_session *ng2s,
                            const nghttp2_frame *frame,
                            void *userp)
{
    h2_session *session = (h2_session *)userp;
    h2_stream *stream;
    apr_status_t rv = APR_SUCCESS;
    
    if (APLOGcdebug(session->c)) {
        char buffer[256];
        
        h2_util_frame_print(frame, buffer, sizeof(buffer)/sizeof(buffer[0]));
        ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, session->c, 
                      H2_SSSN_LOG(APLOGNO(03066), session, 
                      "recv FRAME[%s], frames=%ld/%ld (r/s)"),
                      buffer, (long)session->frames_received,
                     (long)session->frames_sent);
    }

    ++session->frames_received;
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            /* This can be HEADERS for a new stream, defining the request,
             * or HEADER may come after DATA at the end of a stream as in
             * trailers */
            stream = get_stream(session, frame->hd.stream_id);
            if (stream) {
                rv = h2_stream_recv_frame(stream, NGHTTP2_HEADERS, frame->hd.flags, 
                    frame->hd.length + H2_FRAME_HDR_LEN);
            }
            break;
        case NGHTTP2_DATA:
            stream = get_stream(session, frame->hd.stream_id);
            if (stream) {
                ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, session->c,  
                              H2_STRM_LOG(APLOGNO(02923), stream, 
                              "DATA, len=%ld, flags=%d"), 
                              (long)frame->hd.length, frame->hd.flags);
                rv = h2_stream_recv_frame(stream, NGHTTP2_DATA, frame->hd.flags, 
                    frame->hd.length + H2_FRAME_HDR_LEN);
            }
            break;
        case NGHTTP2_PRIORITY:
            session->reprioritize = 1;
            ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                          "h2_stream(%ld-%d): PRIORITY frame "
                          " weight=%d, dependsOn=%d, exclusive=%d", 
                          session->id, (int)frame->hd.stream_id,
                          frame->priority.pri_spec.weight,
                          frame->priority.pri_spec.stream_id,
                          frame->priority.pri_spec.exclusive);
            break;
        case NGHTTP2_WINDOW_UPDATE:
            ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                          "h2_stream(%ld-%d): WINDOW_UPDATE incr=%d", 
                          session->id, (int)frame->hd.stream_id,
                          frame->window_update.window_size_increment);
            if (nghttp2_session_want_write(session->ngh2)) {
                dispatch_event(session, H2_SESSION_EV_FRAME_RCVD, 0, "window update");
            }
            break;
        case NGHTTP2_RST_STREAM:
            ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, session->c, APLOGNO(03067)
                          "h2_stream(%ld-%d): RST_STREAM by client, errror=%d",
                          session->id, (int)frame->hd.stream_id,
                          (int)frame->rst_stream.error_code);
            stream = get_stream(session, frame->hd.stream_id);
            if (stream && stream->initiated_on) {
                ++session->pushes_reset;
            }
            else {
                ++session->streams_reset;
            }
            break;
        case NGHTTP2_GOAWAY:
            if (frame->goaway.error_code == 0 
                && frame->goaway.last_stream_id == ((1u << 31) - 1)) {
                /* shutdown notice. Should not come from a client... */
                session->remote.accepting = 0;
            }
            else {
                session->remote.accepted_max = frame->goaway.last_stream_id;
                dispatch_event(session, H2_SESSION_EV_REMOTE_GOAWAY, 
                               frame->goaway.error_code, NULL);
            }
            break;
        case NGHTTP2_SETTINGS:
            if (APLOGctrace2(session->c)) {
                ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                              H2_SSSN_MSG(session, "SETTINGS, len=%ld"), (long)frame->hd.length);
            }
            break;
        default:
            if (APLOGctrace2(session->c)) {
                char buffer[256];
                
                h2_util_frame_print(frame, buffer,
                                    sizeof(buffer)/sizeof(buffer[0]));
                ap_log_cerror(APLOG_MARK, APLOG_TRACE2, 0, session->c,
                              H2_SSSN_MSG(session, "on_frame_rcv %s"), buffer);
            }
            break;
    }
    
    if (session->state == H2_SESSION_ST_IDLE) {
        /* We received a frame, but session is in state IDLE. That means the frame
         * did not really progress any of the (possibly) open streams. It was a meta
         * frame, e.g. SETTINGS/WINDOW_UPDATE/unknown/etc.
         * Remember: IDLE means we cannot send because either there are no streams open or
         * all open streams are blocked on exhausted WINDOWs for outgoing data.
         * The more frames we receive that do not change this, the less interested we
         * become in serving this connection. This is expressed in increasing "idle_delays".
         * Eventually, the connection will timeout and we'll close it. */
        session->idle_frames = H2MIN(session->idle_frames + 1, session->frames_received);
            ap_log_cerror( APLOG_MARK, APLOG_TRACE2, 0, session->c,
                          H2_SSSN_MSG(session, "session has %ld idle frames"), 
                          (long)session->idle_frames);
        if (session->idle_frames > 10) {
            apr_size_t busy_frames = H2MAX(session->frames_received - session->idle_frames, 1);
            int idle_ratio = (int)(session->idle_frames / busy_frames); 
            if (idle_ratio > 100) {
                session->idle_delay = apr_time_from_msec(H2MIN(1000, idle_ratio));
            }
            else if (idle_ratio > 10) {
                session->idle_delay = apr_time_from_msec(10);
            }
            else if (idle_ratio > 1) {
                session->idle_delay = apr_time_from_msec(1);
            }
            else {
                session->idle_delay = 0;
            }
        }
    }
    
    if (APR_SUCCESS != rv) return NGHTTP2_ERR_PROTO;
    return 0;
}