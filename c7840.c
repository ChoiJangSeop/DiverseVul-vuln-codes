static h2_stream *get_timed_out_busy_stream(h2_mplx *m) 
{
    stream_iter_ctx ctx;
    ctx.m = m;
    ctx.stream = NULL;
    ctx.now = apr_time_now();
    h2_ihash_iter(m->streams, timed_out_busy_iter, &ctx);
    return ctx.stream;
}