static int http_open(URLContext *h, const char *uri, int flags,
                     AVDictionary **options)
{
    HTTPContext *s = h->priv_data;
    int ret;

    if( s->seekable == 1 )
        h->is_streamed = 0;
    else
        h->is_streamed = 1;

    s->filesize = -1;
    s->location = av_strdup(uri);
    if (!s->location)
        return AVERROR(ENOMEM);
    if (options)
        av_dict_copy(&s->chained_options, *options, 0);

    if (s->headers) {
        int len = strlen(s->headers);
        if (len < 2 || strcmp("\r\n", s->headers + len - 2)) {
            av_log(h, AV_LOG_WARNING,
                   "No trailing CRLF found in HTTP header.\n");
            ret = av_reallocp(&s->headers, len + 3);
            if (ret < 0)
                return ret;
            s->headers[len]     = '\r';
            s->headers[len + 1] = '\n';
            s->headers[len + 2] = '\0';
        }
    }

    if (s->listen) {
        return http_listen(h, uri, flags, options);
    }
    ret = http_open_cnx(h, options);
    if (ret < 0)
        av_dict_free(&s->chained_options);
    return ret;
}