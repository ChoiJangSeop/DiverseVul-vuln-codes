static int http_read_stream(URLContext *h, uint8_t *buf, int size)
{
    HTTPContext *s = h->priv_data;
    int err, new_location, read_ret;
    int64_t seek_ret;

    if (!s->hd)
        return AVERROR_EOF;

    if (s->end_chunked_post && !s->end_header) {
        err = http_read_header(h, &new_location);
        if (err < 0)
            return err;
    }

    if (s->chunksize >= 0) {
        if (!s->chunksize) {
            char line[32];

                do {
                    if ((err = http_get_line(s, line, sizeof(line))) < 0)
                        return err;
                } while (!*line);    /* skip CR LF from last chunk */

                s->chunksize = strtoll(line, NULL, 16);

                av_log(NULL, AV_LOG_TRACE, "Chunked encoding data size: %"PRId64"'\n",
                        s->chunksize);

                if (!s->chunksize)
                    return 0;
        }
        size = FFMIN(size, s->chunksize);
    }
#if CONFIG_ZLIB
    if (s->compressed)
        return http_buf_read_compressed(h, buf, size);
#endif /* CONFIG_ZLIB */
    read_ret = http_buf_read(h, buf, size);
    if (   (read_ret  < 0 && s->reconnect        && (!h->is_streamed || s->reconnect_streamed) && s->filesize > 0 && s->off < s->filesize)
        || (read_ret == 0 && s->reconnect_at_eof && (!h->is_streamed || s->reconnect_streamed))) {
        int64_t target = h->is_streamed ? 0 : s->off;

        if (s->reconnect_delay > s->reconnect_delay_max)
            return AVERROR(EIO);

        av_log(h, AV_LOG_INFO, "Will reconnect at %"PRId64" error=%s.\n", s->off, av_err2str(read_ret));
        av_usleep(1000U*1000*s->reconnect_delay);
        s->reconnect_delay = 1 + 2*s->reconnect_delay;
        seek_ret = http_seek_internal(h, target, SEEK_SET, 1);
        if (seek_ret != target) {
            av_log(h, AV_LOG_ERROR, "Failed to reconnect at %"PRId64".\n", target);
            return read_ret;
        }

        read_ret = http_buf_read(h, buf, size);
    } else
        s->reconnect_delay = 0;

    return read_ret;
}