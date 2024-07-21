static h2_stream *get_latest_repeatable_unsubmitted_stream(h2_mplx *m) 
{
    stream_iter_ctx ctx;
    ctx.m = m;
    ctx.stream = NULL;
    h2_ihash_iter(m->streams, latest_repeatable_unsubmitted_iter, &ctx);
    return ctx.stream;
}