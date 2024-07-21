static ssize_t stream_request_data(nghttp2_session *ngh2, int32_t stream_id, 
                                   uint8_t *buf, size_t length,
                                   uint32_t *data_flags, 
                                   nghttp2_data_source *source, void *user_data)
{
    h2_proxy_stream *stream;
    apr_status_t status = APR_SUCCESS;
    
    *data_flags = 0;
    stream = nghttp2_session_get_stream_user_data(ngh2, stream_id);
    if (!stream) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, ap_server_conf, APLOGNO(03361)
                     "h2_proxy_stream(NULL): data_read, stream %d not found", 
                     stream_id);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    
    if (stream->session->check_ping) {
        /* suspend until we hear from the other side */
        stream->waiting_on_ping = 1;
        status = APR_EAGAIN;
    }
    else if (stream->r->expecting_100) {
        /* suspend until the answer comes */
        stream->waiting_on_100 = 1;
        status = APR_EAGAIN;
    }
    else if (APR_BRIGADE_EMPTY(stream->input)) {
        status = ap_get_brigade(stream->r->input_filters, stream->input,
                                AP_MODE_READBYTES, APR_NONBLOCK_READ,
                                H2MAX(APR_BUCKET_BUFF_SIZE, length));
        ap_log_rerror(APLOG_MARK, APLOG_TRACE2, status, stream->r, 
                      "h2_proxy_stream(%s-%d): request body read", 
                      stream->session->id, stream->id);
    }

    if (status == APR_SUCCESS) {
        ssize_t readlen = 0;
        while (status == APR_SUCCESS 
               && (readlen < length)
               && !APR_BRIGADE_EMPTY(stream->input)) {
            apr_bucket* b = APR_BRIGADE_FIRST(stream->input);
            if (APR_BUCKET_IS_METADATA(b)) {
                if (APR_BUCKET_IS_EOS(b)) {
                    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
                }
                else {
                    /* we do nothing more regarding any meta here */
                }
            }
            else {
                const char *bdata = NULL;
                apr_size_t blen = 0;
                status = apr_bucket_read(b, &bdata, &blen, APR_BLOCK_READ);
                
                if (status == APR_SUCCESS && blen > 0) {
                    ssize_t copylen = H2MIN(length - readlen, blen);
                    memcpy(buf, bdata, copylen);
                    buf += copylen;
                    readlen += copylen;
                    if (copylen < blen) {
                        /* We have data left in the bucket. Split it. */
                        status = apr_bucket_split(b, copylen);
                    }
                }
            }
            apr_bucket_delete(b);
        }

        stream->data_sent += readlen;
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, status, stream->r, APLOGNO(03468) 
                      "h2_proxy_stream(%d): request DATA %ld, %ld"
                      " total, flags=%d", stream->id, (long)readlen, (long)stream->data_sent,
                      (int)*data_flags);
        if ((*data_flags & NGHTTP2_DATA_FLAG_EOF) && !apr_is_empty_table(stream->r->trailers_in)) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, status, stream->r, APLOGNO(03468) 
                          "h2_proxy_stream(%d): submit trailers", stream->id);
            *data_flags |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
            submit_trailers(stream);
        } 
        return readlen;
    }
    else if (APR_STATUS_IS_EAGAIN(status)) {
        /* suspended stream, needs to be re-awakened */
        ap_log_rerror(APLOG_MARK, APLOG_TRACE2, status, stream->r, 
                      "h2_proxy_stream(%s-%d): suspending", 
                      stream->session->id, stream_id);
        stream->suspended = 1;
        h2_proxy_iq_add(stream->session->suspended, stream->id, NULL, NULL);
        return NGHTTP2_ERR_DEFERRED;
    }
    else {
        nghttp2_submit_rst_stream(ngh2, NGHTTP2_FLAG_NONE, 
                                  stream_id, NGHTTP2_STREAM_CLOSED);
        return NGHTTP2_ERR_STREAM_CLOSING;
    }
}