apr_status_t h2_stream_send_frame(h2_stream *stream, int ftype, int flags, size_t frame_len)
{
    apr_status_t status = APR_SUCCESS;
    int new_state, eos = 0;

    new_state = on_frame_send(stream->state, ftype);
    if (new_state < 0) {
        ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, stream->session->c, 
                      H2_STRM_MSG(stream, "invalid frame %d send"), ftype);
        AP_DEBUG_ASSERT(new_state > S_XXX);
        return transit(stream, new_state);
    }

    ++stream->out_frames;
    stream->out_frame_octets += frame_len;
    switch (ftype) {
        case NGHTTP2_DATA:
            eos = (flags & NGHTTP2_FLAG_END_STREAM);
            break;
            
        case NGHTTP2_HEADERS:
            eos = (flags & NGHTTP2_FLAG_END_STREAM);
            break;
            
        case NGHTTP2_PUSH_PROMISE:
                /* start pushed stream */
                ap_assert(stream->request == NULL);
                ap_assert(stream->rtmp != NULL);
                status = h2_request_end_headers(stream->rtmp, stream->pool, 1, 0);
                if (status != APR_SUCCESS) {
                    return status;
                }
                set_policy_for(stream, stream->rtmp);
                stream->request = stream->rtmp;
                stream->rtmp = NULL;
            break;
            
        default:
            break;
    }
    ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, stream->session->c, 
                  H2_STRM_MSG(stream, "send frame %d, eos=%d"), ftype, eos);
    status = transit(stream, new_state);
    if (status == APR_SUCCESS && eos) {
        status = transit(stream, on_event(stream, H2_SEV_CLOSED_L));
    }
    return status;
}