static void parse_content_range(URLContext *h, const char *p)
{
    HTTPContext *s = h->priv_data;
    const char *slash;

    if (!strncmp(p, "bytes ", 6)) {
        p     += 6;
        s->off = strtoll(p, NULL, 10);
        if ((slash = strchr(p, '/')) && strlen(slash) > 0)
            s->filesize = strtoll(slash + 1, NULL, 10);
    }
    if (s->seekable == -1 && (!s->is_akamai || s->filesize != 2147483647))
        h->is_streamed = 0; /* we _can_ in fact seek */
}