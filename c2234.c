int am_generate_random_bytes(request_rec *r, void *dest, apr_size_t count)
{
    int rc;
    rc = RAND_pseudo_bytes((unsigned char *)dest, (int)count);
    if(rc == -1) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error generating random data: %lu",
                      ERR_get_error());
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if(rc == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                      "Random data is not cryptographically strong.");
    }

    return OK;
}