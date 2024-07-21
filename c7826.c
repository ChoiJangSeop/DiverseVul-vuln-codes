apr_status_t h2_stream_add_header(h2_stream *stream,
                                  const char *name, size_t nlen,
                                  const char *value, size_t vlen)
{
    h2_session *session = stream->session;
    int error = 0;
    apr_status_t status;
    
    if (stream->has_response) {
        return APR_EINVAL;    
    }
    ++stream->request_headers_added;
    if (name[0] == ':') {
        if ((vlen) > session->s->limit_req_line) {
            /* pseudo header: approximation of request line size check */
            ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c,
                          H2_STRM_MSG(stream, "pseudo %s too long"), name);
            error = HTTP_REQUEST_URI_TOO_LARGE;
        }
    }
    else if ((nlen + 2 + vlen) > session->s->limit_req_fieldsize) {
        /* header too long */
        ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c,
                      H2_STRM_MSG(stream, "header %s too long"), name);
        error = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
    }
    
    if (stream->request_headers_added > session->s->limit_req_fields + 4) {
        /* too many header lines, include 4 pseudo headers */
        if (stream->request_headers_added 
            > session->s->limit_req_fields + 4 + 100) {
            /* yeah, right */
            h2_stream_rst(stream, H2_ERR_ENHANCE_YOUR_CALM);
            return APR_ECONNRESET;
        }
        ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c,
                      H2_STRM_MSG(stream, "too many header lines")); 
        error = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
    }
    
    if (error) {
        set_error_response(stream, error);
        return APR_EINVAL; 
    }
    else if (H2_SS_IDLE == stream->state) {
        if (!stream->rtmp) {
            stream->rtmp = h2_req_create(stream->id, stream->pool, 
                                         NULL, NULL, NULL, NULL, NULL, 0);
        }
        status = h2_request_add_header(stream->rtmp, stream->pool,
                                       name, nlen, value, vlen);
    }
    else if (H2_SS_OPEN == stream->state) {
        status = add_trailer(stream, name, nlen, value, vlen);
    }
    else {
        status = APR_EINVAL;
    }
    
    if (status != APR_SUCCESS) {
        ap_log_cerror(APLOG_MARK, APLOG_TRACE1, 0, session->c,
                      H2_STRM_MSG(stream, "header %s not accepted"), name);
        h2_stream_dispatch(stream, H2_SEV_CANCELLED);
    }
    return status;
}