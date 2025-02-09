static int http_buf_read(URLContext *h, uint8_t *buf, int size)
{
    HTTPContext *s = h->priv_data;
    int len;
    /* read bytes from input buffer first */
    len = s->buf_end - s->buf_ptr;
    if (len > 0) {
        if (len > size)
            len = size;
        memcpy(buf, s->buf_ptr, len);
        s->buf_ptr += len;
    } else {
        int64_t target_end = s->end_off ? s->end_off : s->filesize;
        if ((!s->willclose || s->chunksize < 0) &&
            target_end >= 0 && s->off >= target_end)
            return AVERROR_EOF;
        len = ffurl_read(s->hd, buf, size);
        if (!len && (!s->willclose || s->chunksize < 0) &&
            target_end >= 0 && s->off < target_end) {
            av_log(h, AV_LOG_ERROR,
                   "Stream ends prematurely at %"PRId64", should be %"PRId64"\n",
                   s->off, target_end
                  );
            return AVERROR(EIO);
        }
    }
    if (len > 0) {
        s->off += len;
        if (s->chunksize > 0)
            s->chunksize -= len;
    }
    return len;
}