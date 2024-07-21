static int on_data_chunk_recv_cb(nghttp2_session *ngh2, uint8_t flags,
                                 int32_t stream_id,
                                 const uint8_t *data, size_t len, void *userp)
{
    h2_session *session = (h2_session *)userp;
    apr_status_t status = APR_EINVAL;
    h2_stream * stream;
    int rv = 0;
    
    stream = h2_session_stream_get(session, stream_id);
    if (stream) {
        status = h2_stream_recv_DATA(stream, flags, data, len);
    }
    else {
        ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, session->c, APLOGNO(03064)
                      "h2_stream(%ld-%d): on_data_chunk for unknown stream",
                      session->id, (int)stream_id);
        rv = NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    
    if (status != APR_SUCCESS) {
        /* count this as consumed explicitly as no one will read it */
        nghttp2_session_consume(session->ngh2, stream_id, len);
    }
    return rv;
}